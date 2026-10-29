// Ghidra headless post-script: apply names from analysis/<prog>_names.txt,
// add function starts from analysis/<prog>_funcs.tsv that auto-analysis
// missed, then write every function's decompilation to the output file.
//
//   args: <names.txt> <funcs.tsv> <out.c>
// Run through reverse-engineering/tools/ghidra/decomp.sh.
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class DumpDecomp extends GhidraScript {
    private Address addr(String hex) {
        return currentProgram.getAddressFactory().getDefaultAddressSpace()
            .getAddress(Long.parseLong(hex.replace("0x", ""), 16));
    }

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        Listing listing = currentProgram.getListing();
        for (String line : Files.readAllLines(Paths.get(a[1]))) {
            if (line.startsWith("#")) continue;
            Address at = addr(line.split("\t")[0]);
            if (listing.getFunctionAt(at) == null && listing.getFunctionContaining(at) == null) {
                disassemble(at);
                createFunction(at, null);
            }
        }
        for (String line : Files.readAllLines(Paths.get(a[0]))) {
            line = line.split("#")[0].trim();
            if (line.isEmpty()) continue;
            String[] f = line.split("\\s+");
            Address at = addr(f[0]);
            Function fn = listing.getFunctionAt(at);
            if (fn == null) {
                disassemble(at);
                fn = createFunction(at, f[1]);
            }
            if (fn != null) fn.setName(f[1], SourceType.USER_DEFINED);
        }
        DecompInterface d = new DecompInterface();
        d.openProgram(currentProgram);
        try (PrintWriter out = new PrintWriter(new FileWriter(a[2]))) {
            for (Function fn : listing.getFunctions(true)) {
                DecompileResults r = d.decompileFunction(fn, 60, monitor);
                out.println("// ==== " + fn.getEntryPoint() + " " + fn.getName());
                if (r != null && r.decompileCompleted())
                    out.println(r.getDecompiledFunction().getC());
                else
                    out.println("// decompile failed");
            }
        }
    }
}
