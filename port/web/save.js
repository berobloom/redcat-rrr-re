// The browser's IndexedDB keeps two directories: /data, the game files the
// player chose once (shell.html copies them in), and /save (RRRIO.INF and the
// SMD files). Both are read in before the page decides what to show; game.c
// writes them back with FS.syncfs(false) after each save.
Module.preRun = Module.preRun || [];
Module.preRun.push(function () {
    FS.mkdir('/data');
    FS.mount(IDBFS, {}, '/data');
    FS.mkdir('/save');
    FS.mount(IDBFS, {}, '/save');
    addRunDependency('save');
    FS.syncfs(true, function (e) {
        if (e)
            console.log(e);
        removeRunDependency('save');
    });
});
