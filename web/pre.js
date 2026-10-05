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
  var flushInterval = null;

  function flush() {
    if (Module.nivlrResetting) return;
    if (typeof FS === 'undefined' || !FS.syncfs || !nivlrRestored) return;
    if (syncing) { pending = true; return; }
    syncing = true;
    FS.syncfs(false, function (error) {
      syncing = false;
      if (error) console.error('Noctis IV OM: could not persist data', error);
      if (pending && !Module.nivlrResetting) { pending = false; flush(); }
    });
  }

  // The game autosaves on its own cadence; hiding or closing the tab asks it
  // to save on its next frame, and the current files are flushed right away.
  function requestSave() {
    if (Module.nivlrResetting) return;
    Module.nivlrSaveRequested = true;
    flush();
  }

  Module.nivlrPersist = flush;
  flushInterval = setInterval(flush, 10000);
  document.addEventListener('visibilitychange', function () {
    if (document.visibilityState === 'hidden') requestSave();
  });
  window.addEventListener('pagehide', requestSave);

  Module.resetSavedData = function (onComplete) {
    Module.nivlrResetting = true;
    if (flushInterval) {
      clearInterval(flushInterval);
      flushInterval = null;
    }

    function finish() {
      if (typeof onComplete === 'function') {
        onComplete();
      } else {
        window.location.reload();
      }
    }

    function deleteIndexedDb() {
      try {
        if (typeof IDBFS !== 'undefined' && IDBFS.dbs && IDBFS.dbs['/persistent']) {
          IDBFS.dbs['/persistent'].close();
          delete IDBFS.dbs['/persistent'];
        }
      } catch (e) {
        console.warn('Noctis IV OM: could not close IDBFS database connection', e);
      }

      if (window.indexedDB && window.indexedDB.deleteDatabase) {
        var finished = false;
        function done() {
          if (finished) return;
          finished = true;
          finish();
        }

        try {
          var req = window.indexedDB.deleteDatabase('/persistent');
          req.onsuccess = function () {
            console.log('Noctis IV OM: IndexedDB /persistent cleared.');
            done();
          };
          req.onerror = function (e) {
            console.warn('Noctis IV OM: error deleting IndexedDB /persistent', e);
            done();
          };
          req.onblocked = function () {
            console.warn('Noctis IV OM: IndexedDB /persistent deletion blocked; proceeding.');
            done();
          };
          setTimeout(done, 1000);
        } catch (e) {
          console.warn('Noctis IV OM: exception deleting IndexedDB', e);
          done();
        }
      } else {
        finish();
      }
    }

    if (typeof FS !== 'undefined' && FS.syncfs && nivlrRestored) {
      try {
        function removeDirRecursive(path) {
          if (!FS.analyzePath(path).exists) return;
          var stat = FS.stat(path);
          if (FS.isDir(stat.mode)) {
            var entries = FS.readdir(path);
            for (var i = 0; i < entries.length; i++) {
              var name = entries[i];
              if (name !== '.' && name !== '..') {
                removeDirRecursive(path + '/' + name);
              }
            }
            if (path !== '/persistent') {
              FS.rmdir(path);
            }
          } else {
            FS.unlink(path);
          }
        }
        removeDirRecursive('/persistent');
      } catch (err) {
        console.warn('Noctis IV OM: error clearing files under /persistent', err);
      }
      FS.syncfs(false, function () {
        deleteIndexedDb();
      });
    } else {
      deleteIndexedDb();
    }
  };
})();
