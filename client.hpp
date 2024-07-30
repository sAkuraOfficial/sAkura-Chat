#pragma once
#include "extern.hpp"
#include <boost/asio.hpp>
#include <condition_variable>
#include <hello_imgui/hello_imgui.h>
#include <mutex>
#include <thread>
#include <vector>

using namespace boost::asio;
using namespace boost::asio::ip;

enum Window_List_Mode_
{
    History,
    Friends,
    Search
};
static int Window_List_Mode = Window_List_Mode_::History;

class client_sAkura_Chat
{
  private:
    HelloImGui::DockingParams CreateDockingParams();
    static void CreateFont();
    static void ShowWindow_Control();
    static void ShowWindow_List();
    static void ShowWindow_Chat();
    static void MessageTranslate(); // 客户端消息处理
    // 聊天列表窗口状态

  public:
    client_sAkura_Chat();
    ~client_sAkura_Chat();
};

struct ExampleAppLog
{
    ImGuiTextBuffer Buf;
    ImGuiTextFilter Filter;
    ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool AutoScroll;           // Keep scrolling if already at the bottom.

    ExampleAppLog()
    {
        AutoScroll = true;
        Clear();
    }

    void Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void AddLog(const char *fmt, ...) IM_FMTARGS(2)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void Draw(const char *title, bool *p_open = NULL)
    {
        if (ImGui::BeginChild("scrolling", ImVec2(0, -70), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            // if (clear)
            //     Clear();
            // if (copy)
            //     ImGui::LogToClipboard();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            const char *buf = Buf.begin();
            const char *buf_end = Buf.end();
            if (Filter.IsActive())
            {
                for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
                {
                    const char *line_start = buf + LineOffsets[line_no];
                    const char *line_end =
                        (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    if (Filter.PassFilter(line_start, line_end))
                        ImGui::TextUnformatted(line_start, line_end);
                }
            }
            else
            {
                ImGuiListClipper clipper;
                clipper.Begin(LineOffsets.Size);
                while (clipper.Step())
                {
                    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                    {
                        const char *line_start = buf + LineOffsets[line_no];
                        const char *line_end =
                            (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                        if (line_start != nullptr && line_start[0] != 0)
                        {
                            HelloImGui::ImageFromAsset("zh.jpg", ImVec2(75, 0));
                            ImGui::SameLine();
                        }
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        float warp = ImGui::GetWindowSize().x - 100;
                        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + warp);
                        ImGui::TextUnformatted(line_start, line_end);
                        ImGui::PopTextWrapPos();
                    }
                }
                clipper.End();
            }
            ImGui::PopStyleVar();

            if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }
};
