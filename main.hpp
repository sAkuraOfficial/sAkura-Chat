#pragma once
#include <hello_imgui/hello_imgui.h>
class sAkura_Chat
{
  private:
    HelloImGui::DockingParams CreateDockingParams();
    static void CreateFont();
    static void ShowWindow_Control();
    static void ShowWindow_List();
    static void ShowWindow_Chat();

  public:
    sAkura_Chat();
    ~sAkura_Chat();
};