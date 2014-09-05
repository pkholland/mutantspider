var LibraryMutantspider = {
  ms_back_buffer_clear__sig: 'viii',
  ms_back_buffer_clear: function(red, green, blue) {
    mutantspider.asm_internal.back_buffer_clear(red, green, blue);
  },
  ms_stretch_blit_pixels__sig: 'viiiffff',
  ms_stretch_blit_pixels: function(data, srcWidth, srcHeight, dstOrigX, dstOrigY, xscale, yscale) {
	mutantspider.asm_internal.stretch_blit_pixels(data, srcWidth, srcHeight, dstOrigX, dstOrigY, xscale, yscale);
  },
  ms_paint_back_buffer__sig: 'v',
  ms_paint_back_buffer: function() {
    mutantspider.asm_internal.paint_back_buffer();
  },
  ms_new_http_request__sig: 'i',
  ms_new_http_request: function() {
    return mutantspider.asm_internal.new_http_request();
  },
  ms_delete_http_request__sig: 'vi',
  ms_delete_http_request: function(id) {
    mutantspider.asm_internal.delete_http_request(id);
  },
  ms_open_http_request__sig: 'iiiiii',
  ms_open_http_request: function(id, method, urlAddr, callback, cb_user_data) {
    return mutantspider.asm_internal.open_http_request(id, method, urlAddr, callback, cb_user_data);
  },
  ms_get_http_download_size__sig: 'ii',
  ms_get_http_download_size: function(id) {
    return mutantspider.asm_internal.get_http_download_size(id);
  },
  ms_read_http_response__sig: 'iiii',
  ms_read_http_response: function(id, buffer, bytes_to_read) {
    return mutantspider.asm_internal.read_http_response(id, buffer, bytes_to_read);
  },
  ms_timed_callback__sig: 'viiii',
  ms_timed_callback: function(milli, callbackAddr, user_data, result) {
    mutantspider.asm_internal.timed_callback(milli, callbackAddr, user_data, result);
  },
  ms_post_string_message__sig: 'vi',
  ms_post_string_message: function(msgAddr) {
    mutantspider.asm_internal.post_string_message(msgAddr);
  },
  ms_bind_graphics__sig: 'vii',
  ms_bind_graphics: function(width, height) {
    mutantspider.asm_internal.bind_graphics(width, height);
  },
  ms_initialize__sig: 'v',
  ms_initialize: function() {
    mutantspider.asm_internal.initialize();
  },
  ms_mkdir__sig: 'vi',
  ms_mkdir: function(pathAddr) {
	FS.mkdir(Pointer_stringify(pathAddr));
  },
  ms_persist_mount__sig: 'vi',
  ms_persist_mount__deps: ['$FS', '$PBMEMFS', '$MEMFS', '$IDBFS'],
  ms_persist_mount: function(pathAddr) {
    var path = Pointer_stringify(pathAddr)
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
      if(err)
        mutantspider.post_js_message('#command:async_startup_failed:' + err);
      else
      {
        ccall('MS_AsyncStartupComplete','null',[],[]);
        mutantspider.post_js_message('#command:async_startup_complete:');
      }
    });
  },
  ms_browser_supports_persistent_storage__sig: 'i',
  ms_browser_supports_persistent_storage__deps: ['$IDBFS'],
  ms_browser_supports_persistent_storage: function() {
    return IDBFS.indexedDB() ? 1 : 0;
  }
};

mergeInto(LibraryManager.library, LibraryMutantspider);
