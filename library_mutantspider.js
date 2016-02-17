mergeInto( LibraryManager.library, {
  ms_consolelog__sig: 'vi',
  ms_consolelog: function(msg) {
    if (typeof importScripts === 'function')
      postMessage({api:'consolelog', args:[Pointer_stringify(msg)]});
    else
      console.log(Pointer_stringify(msg));
  },
  ms_get_browser_language__sig: 'i',
  ms_get_browser_language: function() {
    var ptr = Module.ccall('malloc', 'number', ['number'], [Module.__ms_browser_language__.length + 1]);
    for (var p = 0; p < Module.__ms_browser_language__.length; p++)
      Module.HEAP8[ptr+p] = Module.__ms_browser_language__.charCodeAt(p);
    Module.HEAP8[ptr+Module.__ms_browser_language__.length] = 0;
    return ptr;
  },
  ms_persist_mount__sig: 'vi',
  ms_persist_mount__deps: ['$FS', '$PBMEMFS', '$MEMFS', '$IDBFS'],
  ms_persist_mount: function(path_addr) {
    var path = Pointer_stringify(path_addr);
    if (IDBFS.indexedDB())
      FS.mount(PBMEMFS, {}, path);
    else {
      console.log('request for persistent mount on ' + path + ', but this browser does not support IndexedDB so using non-persistent instead');
      FS.mount(MEMFS, {}, path);
    }
  },
  ms_rez_mount__sig: 'vii',
  ms_rez_mount__deps: ['$FS', '$REZFS'],
  ms_rez_mount: function(pathAddr, root_addr) {
      FS.mount(REZFS, {root_addr: root_addr}, Pointer_stringify(pathAddr));
  },
  ms_syncfs_from_persistent__sig: 'v',
  ms_syncfs_from_persistent__deps: ['$FS'],
  ms_syncfs_from_persistent: function() {
    FS.syncfs(true, function(err) {
      if (!err)
        ccall('MS_AsyncStartupComplete','null',[],[]);
      // non web worker must synchronize the async startup
      // so we only do something here in that case
      if (typeof importScripts !== 'function')
        mutantspider._callbacks.async_startup_complete(Module.__ms_module_id__, err);
    });
  },
  ms_browser_supports_persistent_storage__sig: 'i',
  ms_browser_supports_persistent_storage__deps: ['$IDBFS'],
  ms_browser_supports_persistent_storage: function() {
    return IDBFS.indexedDB() ? 1 : 0;
  },
  ms_timed_callback_js__sig: 'vii',
  ms_timed_callback_js: function(milliseconds, proc) {
    setTimeout(function(){Module.ccall('MS_Callback', 'null', ['number'], [proc]);}, milliseconds);
  }
});

