/**
 * Overnight watchdog: in-session self-healing for unattended pi runs.
 *
 * Loaded only by the supervisor (`pi -e ~/.pi/overnight/watchdog.ts`), never globally.
 * Reads its policy from $OVERNIGHT_POLICY (JSON written by overnight.py) and writes
 * counters to $OVERNIGHT_STATE so the supervisor can classify how the pass ended.
 *
 * Heals, in order of escalation:
 *   - policy:   blocks forbidden bash commands and writes outside the project / into protected paths
 *   - repeats:  blocks an identical tool call issued `repeatLimit` times within the window
 *   - errors:   steers after `errorStreak` consecutive failing tool calls
 *   - settle:   when the model tries to stop without committing, with a truncated reply, or with a
 *               tool call written as plain text, pushes it back to work (at most `maxNudges` times)
 *   - budget:   steers to wrap up at `softTurns` or `softDeadline` (epoch ms, set by the supervisor
 *               from the pass timeout), aborts and exits at `hardTurns`
 *   - loops:    exits once `maxRepeatBlocks` repeat blocks pile up, so the supervisor restarts fresh
 *   - hangs:    gives every bash call a timeout (`bashTimeout` s unless the model asks for one, capped at
 *               `bashMaxTimeout`); pi kills the process tree and the pass continues. pi's bash has no
 *               default timeout, so one non-terminating command used to stall the whole pass.
 */
import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";
import { readFileSync, writeFileSync } from "node:fs";
import { resolve, relative, isAbsolute } from "node:path";

interface Policy {
	projectRoot: string;
	startHead: string;
	forbidBash: string[];
	allowWriteOutside: string[];
	protectedPaths: string[];
	repeatWindow: number;
	repeatLimit: number;
	maxRepeatBlocks: number;
	errorStreak: number;
	maxNudges: number;
	softTurns: number;
	softDeadline: number;
	hardTurns: number;
	bashTimeout: number;
	bashMaxTimeout: number;
}

const DEFAULTS: Omit<Policy, "projectRoot" | "startHead"> = {
	forbidBash: [],
	allowWriteOutside: [],
	protectedPaths: [],
	repeatWindow: 12,
	repeatLimit: 3,
	maxRepeatBlocks: 6,
	errorStreak: 5,
	maxNudges: 3,
	softTurns: 120,
	softDeadline: 0,
	hardTurns: 160,
	bashTimeout: 600,
	bashMaxTimeout: 1800,
};

const FAKE_TOOL_CALL = /<tool_call>|<function=|"name"\s*:\s*"(bash|read|edit|write|grep|find|ls)"|```(json|tool)\s*\{\s*"(name|tool)"/;

function textOf(message: any): string {
	if (!message || !Array.isArray(message.content)) return typeof message?.content === "string" ? message.content : "";
	return message.content
		.filter((c: any) => c.type === "text")
		.map((c: any) => c.text)
		.join("\n");
}

function hasToolCall(message: any): boolean {
	return Array.isArray(message?.content) && message.content.some((c: any) => c.type === "toolCall");
}

export default function (pi: ExtensionAPI) {
	const policyPath = process.env.OVERNIGHT_POLICY;
	const statePath = process.env.OVERNIGHT_STATE;
	if (!policyPath) return; // not under the supervisor: do nothing

	const policy: Policy = { ...DEFAULTS, ...JSON.parse(readFileSync(policyPath, "utf8")) };
	const forbid = policy.forbidBash.map((p) => new RegExp(p));
	const root = resolve(policy.projectRoot);

	const state = {
		startedAt: new Date().toISOString(),
		updatedAt: "",
		turns: 0,
		toolCalls: 0,
		toolErrors: 0,
		policyBlocks: 0,
		repeatBlocks: 0,
		nudges: 0,
		errorSteers: 0,
		bashTimeouts: 0,
		lastTool: "",
		lastStopReason: "",
		lastError: "",
		endReason: "",
		blocked: [] as string[],
	};
	const recent: string[] = [];
	let errorRun = 0;
	let softSent = false;
	let lastAssistant: any;

	const save = () => {
		if (!statePath) return;
		state.updatedAt = new Date().toISOString();
		try {
			writeFileSync(statePath, JSON.stringify(state, null, 1));
		} catch {}
	};
	const note = (what: string) => {
		state.blocked.push(what.slice(0, 200));
		if (state.blocked.length > 30) state.blocked.shift();
	};
	const exit = (reason: string, ctx: any) => {
		state.endReason = reason;
		save();
		ctx.abort();
		ctx.shutdown();
	};
	const steer = (content: string) =>
		pi.sendMessage({ customType: "overnight-watchdog", content, display: true }, { deliverAs: "steer" });

	const insideRoot = (p: string) => {
		const rel = relative(root, resolve(root, p));
		return !rel.startsWith("..") && !isAbsolute(rel);
	};
	const pathAllowed = (p: string): string | undefined => {
		const abs = resolve(root, p);
		if (!insideRoot(abs)) {
			if (policy.allowWriteOutside.some((d) => abs.startsWith(resolve(d)))) return undefined;
			return `writes outside the project (${root}) are not allowed`;
		}
		const rel = relative(root, abs);
		const hit = policy.protectedPaths.find((pp) => rel === pp || rel.startsWith(pp.replace(/\/?$/, "/")));
		return hit ? `${hit} is protected in overnight runs` : undefined;
	};

	pi.on("tool_call", async (event, ctx) => {
		state.toolCalls++;
		state.lastTool = event.toolName;
		const input = event.input as Record<string, unknown>;

		if (event.toolName === "bash") {
			const cmd = String(input.command ?? "");
			const hit = forbid.find((re) => re.test(cmd));
			if (hit) {
				state.policyBlocks++;
				note(`bash: ${cmd}`);
				save();
				return {
					block: true,
					reason: `Blocked by overnight policy (matches /${hit.source}/). This command is not allowed unattended; find another way or record the blocker in STATUS.md.`,
				};
			}
			// in-place mutation of tool_call.input is what pi executes; the repeat key below sees it too
			const t = Number(input.timeout);
			if (!(t > 0)) input.timeout = policy.bashTimeout;
			else if (t > policy.bashMaxTimeout) input.timeout = policy.bashMaxTimeout;
		}
		if (event.toolName === "write" || event.toolName === "edit") {
			const p = String(input.path ?? input.file_path ?? "");
			const why = p && pathAllowed(p);
			if (why) {
				state.policyBlocks++;
				note(`${event.toolName}: ${p}`);
				save();
				return { block: true, reason: `Blocked by overnight policy: ${why}.` };
			}
		}

		const key = event.toolName + " " + JSON.stringify(input);
		recent.push(key);
		if (recent.length > policy.repeatWindow) recent.shift();
		const n = recent.filter((k) => k === key).length;
		if (n >= policy.repeatLimit) {
			state.repeatBlocks++;
			note(`repeat x${n}: ${key}`);
			if (state.repeatBlocks >= policy.maxRepeatBlocks) {
				exit("looping", ctx);
				return { block: true, reason: "Too many repeated calls; ending this pass.", terminate: true };
			}
			save();
			return {
				block: true,
				reason: `You already made this exact call ${n - 1} times in your last ${policy.repeatWindow} calls; the result will not change. Use what you got, try a different approach, or mark the item BLOCKED in STATUS.md and move on.`,
			};
		}
		return undefined;
	});

	pi.on("tool_result", async (event) => {
		if (event.isError) {
			state.toolErrors++;
			errorRun++;
			state.lastError = textOf(event).slice(0, 300);
			const timedOut = event.toolName === "bash" && /Command timed out after \d+ seconds/.test(textOf(event));
			if (timedOut) {
				state.bashTimeouts++;
				note(`timeout: ${String(event.input.command ?? "").slice(0, 150)}`);
				save();
				return {
					content: [
						...event.content,
						{
							type: "text",
							text: `\n[overnight watchdog] Killed: did not finish in ${event.input.timeout}s. Usually a loop that never terminates (e.g. a find/scan whose index never advances) or a command waiting on input; fix the command rather than rerunning it. Only if it legitimately needs longer, pass a larger "timeout" (max ${policy.bashMaxTimeout}).`,
						},
					],
				};
			}
			if (errorRun >= policy.errorStreak) {
				errorRun = 0;
				state.errorSteers++;
				steer(
					`${policy.errorStreak} tool calls in a row failed. Stop and think: re-read the last error, check GOTCHAS.md, and change approach. If the current item cannot be done, write why under BLOCKED in STATUS.md, commit, and take the next item.`,
				);
			}
		} else {
			errorRun = 0;
		}
		return undefined;
	});

	pi.on("message_end", async (event) => {
		const m: any = event.message;
		if (m?.role === "assistant") {
			lastAssistant = m;
			state.lastStopReason = m.stopReason ?? "";
			if (m.errorMessage) state.lastError = String(m.errorMessage).slice(0, 300);
		}
		return undefined;
	});

	pi.on("turn_end", async (event, ctx) => {
		state.turns++;
		if (state.turns >= policy.hardTurns) {
			exit("turn_budget", ctx);
			return undefined;
		}
		const late = policy.softDeadline > 0 && Date.now() >= policy.softDeadline;
		if (!softSent && (state.turns >= policy.softTurns || late)) {
			softSent = true;
			steer(
				`${late ? "Time for this iteration is nearly up" : `Turn budget nearly spent (${state.turns}/${policy.hardTurns})`}. Wrap up now: no new analysis. Write findings, update STATUS.md with the handoff, commit, run overnight/check.sh, then stop.`,
			);
		}
		// No ctx.compact() here: a manual compaction aborts the running agent, and in -p mode pi then
		// exits without finishing the pass. pi's automatic compaction (compaction.enabled) does not abort.
		save();
		return undefined;
	});

	pi.on("agent_before_settle", async (event) => {
		if (state.endReason || state.nudges >= policy.maxNudges || event.outcome === "aborted") return undefined;
		let msg = "";
		const text = textOf(lastAssistant);
		if (lastAssistant && !hasToolCall(lastAssistant) && FAKE_TOOL_CALL.test(text)) {
			msg = "Your last reply contains a tool call written as plain text, so nothing ran. Call the tool through the tool-calling interface instead.";
		} else if (lastAssistant?.stopReason === "length") {
			msg = "Your reply was cut off at the output limit. Continue, in smaller steps (shorter writes, one file at a time).";
		} else {
			const head = await pi.exec("git", ["-C", root, "rev-parse", "HEAD"]).catch(() => undefined);
			const dirty = await pi.exec("git", ["-C", root, "status", "--porcelain"]).catch(() => undefined);
			const committed = head && head.stdout.trim() !== policy.startHead;
			if (!committed) {
				msg = dirty?.stdout.trim()
					? "You are stopping with uncommitted changes and no commit this pass. Finish the procedure: make sure the check passes, replace STATUS.md, and commit."
					: "You are stopping without having committed anything this pass. Either make progress on the first HANDOFF item and commit it, or write under BLOCKED in STATUS.md why it cannot be done, commit that, and continue with the next item.";
			}
		}
		if (!msg) return undefined;
		state.nudges++;
		save();
		return {
			continue: true,
			entries: [{ type: "custom_message", customType: "overnight-watchdog", content: msg, display: true }],
		};
	});

	pi.on("session_shutdown", async () => {
		if (!state.endReason) state.endReason = "exit";
		save();
	});
}
