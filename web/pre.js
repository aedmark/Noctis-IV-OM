// Noctis IV OM browser runtime glue, prepended to the generated module.
//
// Player data (saves, catalogs, gallery, movies, settings) lives under
// /persistent, which is backed by IndexedDB so it survives page reloads.
// The game writes through the ordinary filesystem API; this file restores
// the mount before main() and flushes it back periodically and on page hide.

// Never write back before the saved data has been restored, or an empty
// mount would overwrite the player's IndexedDB copy.
var nivlrRestored = false;

Module.preRun = Module.preRun || [];
Module.preRun.push(function () {
  FS.mkdir('/persistent');
  FS.mount(IDBFS, {}, '/persistent');
  addRunDependency('nivlr-persistent');
  FS.syncfs(true, function (error) {
    if (error) console.error('Noctis IV OM: could not restore saved data', error);
    else nivlrRestored = true;
    removeRunDependency('nivlr-persistent');
  });
});

(function () {
  var syncing = false;
  var pending = false;

  function flush() {
    if (typeof FS === 'undefined' || !FS.syncfs || !nivlrRestored) return;
    if (syncing) { pending = true; return; }
    syncing = true;
    FS.syncfs(false, function (error) {
      syncing = false;
      if (error) console.error('Noctis IV OM: could not persist data', error);
      if (pending) { pending = false; flush(); }
    });
  }

  // The game autosaves on its own cadence; hiding or closing the tab asks it
  // to save on its next frame, and the current files are flushed right away.
  function requestSave() {
    Module.nivlrSaveRequested = true;
    flush();
  }

  Module.nivlrPersist = flush;
  setInterval(flush, 10000);
  document.addEventListener('visibilitychange', function () {
    if (document.visibilityState === 'hidden') requestSave();
  });
  window.addEventListener('pagehide', requestSave);
})();
