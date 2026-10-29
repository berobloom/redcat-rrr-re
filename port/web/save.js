// The save directory /save (RRRIO.INF and the SMD files) lives in the
// browser's IndexedDB: read it in before main() runs; game.c writes it
// back with FS.syncfs(false) after each save.
Module.preRun = Module.preRun || [];
Module.preRun.push(function () {
    FS.mkdir('/save');
    FS.mount(IDBFS, {}, '/save');
    addRunDependency('save');
    FS.syncfs(true, function (e) {
        if (e)
            console.log(e);
        removeRunDependency('save');
    });
});
