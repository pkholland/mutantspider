
#include "mutantspider.h"

#if defined(__native_client__)

#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_embedder.h"

void ms_free_transfered_buffer(ms_transfered_buffer* tb, void* ptr)
{
  auto tb_ = (pp::VarArrayBuffer*)tb;
  tb_->Unmap();
  delete tb_;
}

class msinstance : public pp::Instance
{
public:
  explicit msinstance(PP_Instance instance)
    : pp::Instance(instance)
  {
    initDispatchMap();
  }
  
  bool Init(uint32_t argc, const char* argn[], const char* argv[])
  {
    for (uint32_t i = 0; i < argc; i++) {
      if (!strcmp(argn[i], "browser_language"))
        MS_SetLocale(argv[i]);
      else if (!strcmp(argn[i], "ms_module_id"))
        gModuleID = atoi(argv[i]);
    }
    MS_Init("");
    return true;
  }
  
  void HandleMessage(const pp::Var& var_message)
  {
    if (var_message.is_dictionary())
    {
      pp::VarDictionary d(var_message);
      auto it = map_.find(d.Get("api").AsString());
      if (it != map_.end()) 
        it->second(d);
    }
  }
  
  void initDispatchMap();
  
  std::map<std::string, void (*)(const pp::VarDictionary&)> map_;
};

class msmodule : public pp::Module
{
public:
  virtual pp::Instance* CreateInstance(PP_Instance inst)
  {
    return gGlobalPPInstance = new msinstance(inst);
  }
};

pp::Module* pp::CreateModule()
{
  return new msmodule();
}

#else

void ms_free_transfered_buffer(ms_transfered_buffer* tb, void* ptr)
{
  free(ptr);
}


#endif // else of defined(__native_client__)

