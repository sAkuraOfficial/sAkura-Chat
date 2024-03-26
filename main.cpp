#include "main.hpp"
#include <hello_imgui/hello_imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <misc/freetype/imgui_freetype.h>
int main()
{
    sAkura_Chat app;
    return 0;
}

HelloImGui::DockingParams sAkura_Chat::CreateDockingParams()
{
    const std::string MainDockSpaceName = "MainDockSpace";
    HelloImGui::DockingParams DockingParams;
    DockingParams.layoutCondition = HelloImGui::DockingLayoutCondition::ApplicationStart;

    DockingParams.layoutReset = true;
    DockingParams.mainDockSpaceNodeFlags = ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResize;
    // 左侧控制栏
    {
        HelloImGui::DockingSplit ds_Control;
        ds_Control.initialDock = MainDockSpaceName;
        ds_Control.newDock = "ds_Control";
        ds_Control.direction = ImGuiDir_Left;
        ds_Control.ratio = 0.11f;
        ds_Control.nodeFlags = ImGuiDockNodeFlags_NoTabBar;
        DockingParams.dockingSplits.push_back(ds_Control);

        HelloImGui::DockableWindow dw_Control;
        dw_Control.label = "dw_Control";
        dw_Control.dockSpaceName = "ds_Control";
        dw_Control.GuiFunction = ShowWindow_Control;
        DockingParams.dockableWindows.push_back(dw_Control);
    }
    // 中侧聊天记录,好友列表
    {
        HelloImGui::DockingSplit ds_List;
        ds_List.initialDock = MainDockSpaceName;
        ds_List.newDock = "ds_List";
        ds_List.direction = ImGuiDir_Left;
        ds_List.ratio = 0.51;
        ds_List.nodeFlags = ImGuiDockNodeFlags_NoTabBar;
        DockingParams.dockingSplits.push_back(ds_List);

        HelloImGui::DockableWindow dw_List;
        dw_List.label = "dw_List";
        dw_List.dockSpaceName = "ds_List";
        dw_List.GuiFunction = ShowWindow_List;
        DockingParams.dockableWindows.push_back(dw_List);
    }
    // 右侧聊天界面
    {
        HelloImGui::DockableWindow dw_Chat;
        dw_Chat.label = "dw_Chat";
        dw_Chat.dockSpaceName = MainDockSpaceName;
        dw_Chat.GuiFunction = ShowWindow_Chat;
        DockingParams.dockableWindows.push_back(dw_Chat);
    }
    return DockingParams;
}

void sAkura_Chat::CreateFont()
{
    ImFont temp1 = {};
    ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/微软雅黑.ttf", 100, NULL,
                                             ImGui::GetIO().Fonts->GetGlyphRangesChineseFull());
    static ImWchar ranges[] = {0x1, 0x1FFFF, 0}; // 最后的0是用来标记结尾的
    static ImFontConfig cfg;
    cfg.OversampleH = cfg.OversampleV = 1;
    cfg.MergeMode = true;
    cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
    ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/seguiemj-1.45-3d.ttf", 80.0f, &cfg, ranges);
}

sAkura_Chat::sAkura_Chat()
{
    HelloImGui::RunnerParams p;
    p.appWindowParams.windowGeometry.size = {1100, 660};
    p.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;
    p.appWindowParams.restorePreviousGeometry = false;
    // p.imGuiWindowParams.backgroundColor = ImVec4(0.909f, 0.736f, 1.000f, 0.884f);
    p.imGuiWindowParams.enableViewports = true;
    p.imGuiWindowParams.rememberStatusBarSettings = false;
    p.imGuiWindowParams.rememberTheme = false;
    p.rememberSelectedAlternativeLayout = false;

    p.appWindowParams.borderless = true;
    p.appWindowParams.borderlessHighlightColor = ImVec4(0, 0, 0, 0);
    p.rememberSelectedAlternativeLayout = false;
    p.dockingParams = CreateDockingParams();
    p.callbacks.LoadAdditionalFonts = CreateFont;

    HelloImGui::Run(p);
}

sAkura_Chat::~sAkura_Chat()
{
}

void sAkura_Chat::ShowWindow_Control()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // 设置按钮底色
    HelloImGui::ImageButtonFromAsset("zh.jpg", ImVec2(75, 0));  // 头像显示
    ImGui::NewLine();                                           // 换行
    HelloImGui::ImageButtonFromAsset("ui/liaotian.png", ImVec2(75, 0));
    ImGui::SetWindowFontScale(0.7); // 设置字体大小
    static std::string title_chat = "聊天", title_list = "通讯录", title_search = "搜索";
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(title_chat.c_str()).x) / 2); // 文本居中显示
    ImGui::Text("聊天");
    HelloImGui::ImageButtonFromAsset("ui/mingdan.png", ImVec2(75, 0));
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(title_list.c_str()).x) / 2);
    ImGui::Text("通讯录");
    HelloImGui::ImageButtonFromAsset("ui/sousuo.png", ImVec2(75, 0));
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(title_search.c_str()).x) / 2);
    ImGui::Text("搜索");

    ImGui::SetWindowFontScale(1.0);
    ImGui::PopStyleColor();
}

void sAkura_Chat::ShowWindow_List()
{
    static char buffer_search[1024] = {0};
    ImVec2 ListSize = ImGui::GetWindowSize();
    ImGui::SetNextItemWidth(ListSize.x * 4 / 5 - 20);
    ImGui::InputText("##InputText_search", buffer_search, sizeof(buffer_search));
    ImGui::SameLine();
    ImGui::Button("➕##Button_search", ImVec2(ListSize.x * 1 / 5 - 30, 0));
    std::vector<std::string> id = {"周子豪", "林奕岚", "王嘉俊", "尹俊镔","彭佳伟","邓嘉俊"};
    std::vector<std::string> img = {"zh.jpg", "yl.jpg", "wjj.jpg", "yjb.jpg"};
    for (int i1 = 0; i1 < 4; i1++)
    {
        if (ImGui::BeginChild(std::string("##Child" + i1).c_str(), {ListSize.x - 30, 150}, ImGuiChildFlags_Border))
        {
            HelloImGui::ImageFromAsset(img[i1].c_str(), {100, 0});
            ImGui::SameLine();
            ImVec2 pos = ImGui::GetCursorPos();
            ImGui::Text("%s", id[i1].c_str());

            ImGui::SetCursorPos({pos.x - 5, ImGui::CalcTextSize("你好").y + 10});
            ImGui::SetWindowFontScale(0.6);
            ImGui::Text("你吃了吗?😄"); 
            ImGui::SetWindowFontScale(1.0);

            ImGui::EndChild();
        }
    }
}

void sAkura_Chat::ShowWindow_Chat()
{
    ImGui::GetIO().FontGlobalScale = 0.5;
    ImGui::SetWindowFontScale(1.0);
    ImGui::Text("未命名群聊(3)");
    ImGui::SetWindowFontScale(0.5);
    ImGui::Separator();

}