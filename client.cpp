#include "client.hpp"
// #include <boost/asio.hpp>
#include "extern.hpp"
#include <boost/json.hpp>
#include <hello_imgui/hello_imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <misc/freetype/imgui_freetype.h>

const int MAX_LENGTH = 4096; // send size max

std::vector<std::string> incoming_messages; // 存放收到的消息
std::vector<std::string> outgoing_messages; // 存放要发送的消息
std::mutex mtx;
std::condition_variable cv;

bool server_ok = false; // 是否连接到服务器
bool server_reConnect = false;
bool client_login = false;
static char buffer_login_id[1024] = {0};
static char buffer_login_pwd[1024] = {0};
static bool GPU_MODE = false; // true:独立显卡.false:核心显卡
ExampleAppLog message_window;
char *reply = nullptr;
inline bool Client();

int main()
{
    Client();
    client_sAkura_Chat app;
    return 0;
}

HelloImGui::DockingParams client_sAkura_Chat::CreateDockingParams()
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
        // ds_Control.ratio = 0.11f;
        ds_Control.ratio = 0.06f;
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
        // ds_List.ratio = 0.51f;
        ds_List.ratio = 0.28f;
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

void client_sAkura_Chat::CreateFont()
{
    ImFont temp1 = {};
    ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/微软雅黑.ttf", GPU_MODE ? 80 : 60, NULL, ImGui::GetIO().Fonts->GetGlyphRangesChineseFull());
    static ImWchar ranges[] = {0x1, 0x1FFFF, 0}; // 最后的0是用来标记结尾的
    static ImFontConfig cfg;
    cfg.OversampleH = cfg.OversampleV = 1;
    cfg.MergeMode = true;
    cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
    ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/seguiemj-1.45-3d.ttf", 80.0f, &cfg, ranges);
}

client_sAkura_Chat::client_sAkura_Chat()
{

    HelloImGui::RunnerParams p;
    p.fpsIdling = {9.0f, false, false};
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
    p.dockingParams = CreateDockingParams();
    p.callbacks.LoadAdditionalFonts = CreateFont;
    p.callbacks.SetupImGuiStyle = []() {
        ImGui::StyleColorsLight();
    };
    HelloImGui::Run(p);
}

client_sAkura_Chat::~client_sAkura_Chat()
{
}

void client_sAkura_Chat::MessageTranslate()
{
    try
    {
        auto &temp = incoming_messages.begin();
        while (temp != incoming_messages.end())
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto &i = *temp;
            auto msg = boost::json::parse(i).as_object();
            if (msg["type"] == "send_message")
            {
                // 接受消息
                if (msg.at("sender").at("sender_ID").as_string().compare(buffer_login_id) == 0) // 自己发的消息
                {
                }
                else
                {
                    message_window.AddLog("%s:", msg.at("sender").at("sender_ID").as_string().c_str());
                    message_window.AddLog("%s\n", msg.at("message").as_string().c_str());
                }
            }
            else if (msg["type"] == "AI_RESULT")
            {
                auto message = msg["message"];
                auto choices = message.as_object().at("choices").as_array();
                auto i_ = choices[0].as_object().at("message").at("content").as_string();
                message_window.AddLog("%s:", msg.at("sender").at("sender_ID").as_string().c_str());
                message_window.AddLog("%s\n", i_.c_str());
            }
            incoming_messages.erase(temp);
            cv.notify_one(); // 通知发送线程有新消息
            if (incoming_messages.empty())
                break;
        }
    }
    catch (boost::exception &ec)
    {
        std::cout << "error msg trans!" << std::endl;
    }
}

void client_sAkura_Chat::ShowWindow_Control()
{

    MessageTranslate(); // 处理消息
    // ImGui::ShowDemoWindow();
    static std::string text_login_title = "登录";
    static std::string text_login_welcome = "您还未登录,请先登录!";
    static std::string text_login_switch_account = "您已经登录了,请问是否需要切换账号?";
    static std::string text_login_server_not_ok = "服务器连接失败!请问是否需要尝试重连?";

    static int loop_count = 0; // 循环计数器
    if (loop_count == 0)
    {
        loop_count++;
    }
    else if (loop_count == 1)
    {
        loop_count++;
        ImGui::OpenPopup(text_login_title.c_str()); // 登录窗口
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));    // 设置按钮底色
    if (HelloImGui::ImageButtonFromAsset("zh.jpg", ImVec2(75, 0))) // 头像显示
    {
        ImGui::OpenPopup(text_login_title.c_str()); // 登录窗口
    }
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(text_login_title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (server_ok == true && client_login == false)
        {
            // 未登录
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.880f, 0.961f, 0.772f, 0.500f));
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(text_login_welcome.c_str()).x) / 2);
            ImGui::Text("%s", text_login_welcome.c_str());
            ImGui::Separator();
            ImGui::Text("账号:");
            ImGui::SameLine();
            ImGui::InputText("##InputText_login_id", buffer_login_id, sizeof(buffer_login_id));
            ImGui::Text("密码:");
            ImGui::SameLine();
            ImGui::InputText("##InputText_login_pwd", buffer_login_pwd, sizeof(buffer_login_pwd), ImGuiInputTextFlags_Password);
            ImGui::PopStyleColor();
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 200) / 2);
            if (ImGui::Button("确认登录!", {200, 0}))
            {
                boost::json::object val;
                val["type"] = "login";
                val["id"] = buffer_login_id;
                val["pwd"] = buffer_login_pwd;
                outgoing_messages.push_back(boost::json::serialize(val));
                cv.notify_one();
                client_login = true;
                ImGui::CloseCurrentPopup();
            }
        }
        else if (server_ok == true && client_login == true)
        {
            // 询问是否切换账号
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.880f, 0.961f, 0.772f, 0.500f));
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(text_login_switch_account.c_str()).x) / 2);
            ImGui::Text("%s", text_login_switch_account.c_str());
            ImGui::Separator();
            ImGui::Text("账号:");
            ImGui::SameLine();
            ImGui::InputText("##InputText_login_id", buffer_login_id, sizeof(buffer_login_id));
            ImGui::Text("密码:");
            ImGui::SameLine();
            ImGui::InputText("##InputText_login_pwd", buffer_login_pwd, sizeof(buffer_login_pwd), ImGuiInputTextFlags_Password);
            ImGui::PopStyleColor();
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 200 * 2) / 2);
            if (ImGui::Button("取消切换!", {200, 0}))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("确认登录!", {200, 0}))
            {
                boost::json::object val;
                val["type"] = "login";
                val["id"] = buffer_login_id;
                val["pwd"] = buffer_login_pwd;
                outgoing_messages.push_back(boost::json::serialize(val));
                cv.notify_one();
                client_login = true;
                ImGui::CloseCurrentPopup();
            }
        }
        else if (server_ok == false)
        {
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(text_login_server_not_ok.c_str()).x) / 2);
            ImGui::Text("%s", text_login_server_not_ok.c_str());
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.880f, 0.961f, 0.772f, 0.500f));
            ImGui::Text("账号:");
            ImGui::SameLine();
            ImGui::InputText("##InputText_login_id", buffer_login_id, sizeof(buffer_login_id));
            ImGui::Text("密码:");
            ImGui::SameLine();
            ImGui::InputText("##InputText_login_pwd", buffer_login_pwd, sizeof(buffer_login_pwd), ImGuiInputTextFlags_Password);
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 200 * 2) / 2);
            if (ImGui::Button("取消重连!", {200, 0}))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("尝试重连!", {200, 0}))
            {
                server_ok = Client();
                boost::json::object val;
                val["type"] = "login";
                val["id"] = buffer_login_id;
                val["pwd"] = buffer_login_pwd;
                outgoing_messages.push_back(boost::json::serialize(val));
                cv.notify_one();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::NewLine(); // 换行
    if (HelloImGui::ImageButtonFromAsset("ui/liaotian.png", ImVec2(75, 0)))
    {
        Window_List_Mode = Window_List_Mode_::History;
    }
    ImGui::SetWindowFontScale(GPU_MODE ? 0.7 : 1.0); // 设置字体大小
    static std::string title_chat = "聊天", title_list = "通讯录", title_search = "搜索";
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(title_chat.c_str()).x) / 2); // 文本居中显示
    ImGui::Text("聊天");
    if (HelloImGui::ImageButtonFromAsset("ui/mingdan.png", ImVec2(75, 0)))
    {
        Window_List_Mode = Window_List_Mode_::Friends;
    }
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(title_list.c_str()).x) / 2);
    ImGui::Text("通讯录");
    if (HelloImGui::ImageButtonFromAsset("ui/sousuo.png", ImVec2(75, 0)))
    {
        Window_List_Mode = Window_List_Mode_::Search;
    }
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(title_search.c_str()).x) / 2);
    ImGui::Text("搜索");

    ImGui::SetWindowFontScale(1.0);
    ImGui::PopStyleColor();
}

void client_sAkura_Chat::ShowWindow_List()
{
    static char buffer_search[1024] = {0};
    ImVec2 ListSize = ImGui::GetWindowSize();

    if (server_ok)
    {
        ImGui::Text("服务器状态:✅");
    }
    else
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("服务器状态:❎");
        ImGui::SameLine();
        if (ImGui::Button("尝试重连"))
        {

            server_reConnect = Client();
        }
    }

    ImGui::SetNextItemWidth(ListSize.x * 4 / 5 - 20);
    ImGui::InputText("##InputText_search", buffer_search, sizeof(buffer_search));
    ImGui::SameLine();
    ImGui::Button("➕##Button_search", ImVec2(ListSize.x * 1 / 5 - 30, 0));
    if (Window_List_Mode == Window_List_Mode_::History)
    {

        std::vector<std::string> id = {"周子豪", "周柏豪", "彭佳伟", "邓嘉俊"};
        std::vector<std::string> img = {"zh.jpg", "yjb.jpg"};
        for (int i1 = 0; i1 < 2; i1++)
        {
            if (ImGui::BeginChild(std::string("##Child" + i1).c_str(), {ListSize.x - 30, 150}, ImGuiChildFlags_Border))
            {
                HelloImGui::ImageFromAsset(img[i1].c_str(), {100, 0});
                ImGui::SameLine();
                ImVec2 pos = ImGui::GetCursorPos();
                ImGui::Text("%s", id[i1].c_str());

                ImGui::SetCursorPos({pos.x - 5, ImGui::CalcTextSize("你好").y + 10});
                ImGui::SetWindowFontScale(GPU_MODE ? 0.6 : 1.0);
                ImGui::Text("你吃了吗?😄");
                ImGui::SetWindowFontScale(1.0);

                ImGui::EndChild();
            }
        }
    }
    else if (Window_List_Mode == Window_List_Mode_::Friends)
    {
        ImGui::Text("Friends");
    }
    else if (Window_List_Mode == Window_List_Mode_::Search)
    {
        ImGui::Text("Search");
    }
}

void client_sAkura_Chat::ShowWindow_Chat()
{
    ImGui::GetIO().FontGlobalScale = 0.5;
    ImGui::SetWindowFontScale(1.0);
    ImGui::Text("未命名群聊(3)");
    ImGui::Separator();
    static char buffer_message[2048] = {0};
    message_window.Draw("chat");
    ImGui::SetNextItemWidth(-150 * 2);
    ImGui::InputText("##message", buffer_message, sizeof(buffer_message));
    ImGui::SameLine();
    if (ImGui::Button("发送", ImVec2(130, 0)))
    {
        if (buffer_message[0] != 0)
        {
            message_window.AddLog("%s", buffer_message);
            message_window.AddLog("%s", "\n");
            boost::json::object val;
            val["type"] = "send_message";
            val["sender"] = {
                {"sender_ID", buffer_login_id}
            };
            val["message"] = buffer_message;
            outgoing_messages.push_back(boost::json::serialize(val));
            cv.notify_one();
            buffer_message[0] = 0;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("问AI", ImVec2(130, 0)))
    {
        if (buffer_message[0] != 0)
        {
            message_window.AddLog("%s", buffer_message);
            message_window.AddLog("%s", "\n");
            boost::json::object val;
            val["type"] = "get_AI";
            val["sender"] = {
                {"sender_ID", buffer_login_id}
            };
            val["message"] = buffer_message;
            outgoing_messages.push_back(boost::json::serialize(val));
            cv.notify_one();
            buffer_message[0] = 0;
        }
    }
    ImGui::SetWindowFontScale(0.5);
}

// void receiveMessages(ip::tcp::socket &sock)
//{
//     try
//     {
//         reply = new char[MAX_LENGTH];
//         sock.async_read_some(
//             buffer(reply, MAX_LENGTH),
//             std::bind(
//                 read_handle,
//                 std::placeholders::_1,
//                 std::placeholders::_2
//             )
//         );
//     }
//     catch (boost::system::system_error &e)
//     {
//         std::cerr << e.what() << '\n';
//     }
// }
//
// void read_handle(const boost::system::error_code &error, std::size_t bytes_transferred)
//{
//     if (!error)
//     {
//         std::lock_guard<std::mutex> lock(mtx);
//         incoming_messages.push_back(std::string(reply, bytes_transferred));
//         cv.notify_one(); // 通知发送线程有新消息
//         delete[] reply;
//         receiveMessages(sock);
//     }
//     else
//     {
//         std::cout << "read error" << std::endl;
//         std::cerr << error.what() << '\n';
//     }
// }

// void receiveMessages(ip::tcp::socket &sock)
//{
//     try
//     {
//         auto reply = new char[MAX_LENGTH];
//         /*  sock.async_receive(
//               boost::asio::buffer(reply, MAX_LENGTH),
//               [reply, &sock](const boost::system::error_code &error, std::size_t bytes_transferred) {
//                   if (!error)
//                   {
//                       std::lock_guard<std::mutex> lock(mtx);
//                       incoming_messages.push_back(std::string(reply, bytes_transferred));
//                       cv.notify_one();
//                       delete[] reply;
//                       std::thread receive_thread(receiveMessages, std::ref(sock));
//                       receive_thread.detach();
//                   }
//                   else
//                   {
//                       std::cerr << "Read error: " << error.message() << '\n';
//                       delete[] reply;
//                   }
//               }
//           );*/
//         read(
//             sock,
//             buffer(reply, MAX_LENGTH)
//         );
//         std::lock_guard<std::mutex> lock(mtx);
//         incoming_messages.push_back(std::string(reply));
//         cv.notify_one();
//         delete[] reply;
//         std::thread receive_thread(receiveMessages, std::ref(sock));
//         receive_thread.detach();
//     }
//
//     catch (boost::system::system_error &e)
//     {
//         std::cerr << e.what() << '\n';
//     }
// }

void receiveMessages(ip::tcp::socket &sock)
{
    try
    {
        while (true)
        {
            size_t reply_length = {};
            read(sock, buffer(reinterpret_cast<char *>(&reply_length), sizeof(reply_length)));

            char reply[MAX_LENGTH] = {0};
            read(sock, buffer(reply, reply_length));

            {
                std::lock_guard<std::mutex> lock(mtx);
                incoming_messages.push_back(std::string(reply, reply_length));
            }

            cv.notify_one(); // 通知发送线程有新消息
        }
    }
    catch (boost::system::system_error &e)
    {
        std::cerr << e.what() << '\n';
    }
}

void sendMessages(ip::tcp::socket &sock)
{
    try
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [] {
                return !outgoing_messages.empty();
            }); // 等待有消息可发送

            std::string message = outgoing_messages.back();
            outgoing_messages.pop_back();

            size_t request_length = message.length();
            // write(sock, buffer(reinterpret_cast<char*>(&request_length), sizeof(request_length)));
            write(sock, buffer(message.c_str(), request_length));
        }
    }
    catch (boost::system::system_error &e)
    {
        std::cerr << e.what() << '\n';
    }
}

inline bool Client()
{
    try
    {
        static io_context ioc;
        static ip::tcp::socket sock(ioc);
        static ip::tcp::endpoint remote_ep(ip::address::from_string("127.0.0.1"), 10086);

        boost::system::error_code ec = error::host_not_found; // 找不到主机
        sock.connect(remote_ep, ec);
        if (ec)
        {
            server_ok = false;
            std::cout << "connect failed, code is " << ec.value() << " Message: " << ec.message() << std::endl;
            MessageBox(0, ec.message().c_str(), NULL, NULL);
            return false;
        }
        server_ok = true;
        std::thread receive_thread(receiveMessages, std::ref(sock));
        std::thread send_thread(sendMessages, std::ref(sock));

        receive_thread.detach();
        send_thread.detach();
        return true;
    }
    catch (boost::system::system_error &e)
    {
        std::cerr << e.what() << '\n';
    }
}