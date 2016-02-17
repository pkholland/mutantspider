if (typeof importScripts === 'function') {
  var boot = function(e) {
    if (e.data.api === 'MS_LaunchWorker')  {
      self.removeEventListener('message', boot, false);
      self[e.data.asm_js_module_name]({ __ms_module_id__: e.data.mod_id,
                                        __ms_browser_language__: e.data.browser_language,
                                        TOTAL_MEMORY: e.data.asm_js_memory });
    }
    if (!self["performance"])
      self["performance"] = {};
    if (!self["performance"]["now"])
      self["performance"]["now"] = function() { return Date.now(); }
  };
  self.addEventListener('message', boot, false);
}
