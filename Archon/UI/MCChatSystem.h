#pragma once

#include <vector>
#include <string>
#include <Windows.h>
#include <stdio.h>
#include <algorithm>
#include <cmath>
#include <mutex>

#include "../imgui/imgui_internal.h"
#include "../modules/impl/ChatSystem.h"
#include "../Utils/Utils.h"
#include <imm.h>

#include "../Utils/InputUtils.h"
#include "../Utils/World/FTickCallbackSystem.h"
#include "../Utils/World/TimerHandler.h"


struct ChatMessage
{
    std::string Text;
    float Timestamp;
    float Alpha;
};

class MCChatSystem
{
public:

private:
    std::mutex m_MsgMutex;

    std::vector<ChatMessage> Messages;
    char InputBuf[1024];
    bool IsChatOpen;
    bool ScrollToBottom;
    bool ReclaimFocus;
    bool JustOpened;
    bool IsUserScroll;

    ImVec2 m_WindowSize;

    void HandleWindowEdges()
    {
        if (!IsChatOpen) return;
        auto chatSystem = ModuleManager::Get().GetModule<ChatSystem>();

        ImVec2 win_pos = ImGui::GetWindowPos();
        ImVec2 win_size = m_WindowSize;
        float thickness = 8.0f;
        float min_w = 200.0f;
        float min_h = 100.0f;

        ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);

        ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;

        ImGui::SetCursorScreenPos(ImVec2(win_pos.x + thickness, win_pos.y + win_size.y - thickness));
        ImGui::InvisibleButton(xorstr_("##ResizeBottom"), ImVec2(win_size.x - thickness * 2, thickness));
        if (ImGui::IsItemActive()) m_WindowSize.y = ImMax(m_WindowSize.y + mouse_delta.y, min_h);
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

        ImGui::SetCursorScreenPos(ImVec2(win_pos.x + win_size.x - thickness, win_pos.y + thickness));
        ImGui::InvisibleButton(xorstr_("##ResizeRight"), ImVec2(thickness, win_size.y - thickness * 2));
        if (ImGui::IsItemActive()) m_WindowSize.x = ImMax(m_WindowSize.x + mouse_delta.x, min_w);
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        ImGui::SetCursorScreenPos(ImVec2(win_pos.x, win_pos.y + thickness));
        ImGui::InvisibleButton(xorstr_("##ResizeLeft"), ImVec2(thickness, win_size.y - thickness * 2));
        if (ImGui::IsItemActive())
        {
            float new_w = m_WindowSize.x - mouse_delta.x;
            if (new_w >= min_w)
            {
                ImGui::SetWindowPos(ImVec2(win_pos.x + mouse_delta.x, win_pos.y));
                m_WindowSize.x = new_w;
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        ImGui::SetCursorScreenPos(ImVec2(win_pos.x + thickness, win_pos.y));
        ImGui::InvisibleButton(xorstr_("##ResizeTop"), ImVec2(win_size.x - thickness * 2, thickness));
        if (ImGui::IsItemActive())
        {
            float new_h = m_WindowSize.y - mouse_delta.y;
            if (new_h >= min_h)
            {
                ImGui::SetWindowPos(ImVec2(win_pos.x, win_pos.y + mouse_delta.y));
                m_WindowSize.y = new_h;
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

        ImGui::SetCursorScreenPos(ImVec2(win_pos.x + win_size.x - thickness, win_pos.y + win_size.y - thickness));
        ImGui::InvisibleButton(xorstr_("##ResizeBR"), ImVec2(thickness, thickness));
        if (ImGui::IsItemActive())
        {
            m_WindowSize.x = ImMax(m_WindowSize.x + mouse_delta.x, min_w);
            m_WindowSize.y = ImMax(m_WindowSize.y + mouse_delta.y, min_h);
        }
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);

        ImGui::SetCursorScreenPos(ImVec2(win_pos.x, win_pos.y + win_size.y - thickness));
        ImGui::InvisibleButton(xorstr_("##ResizeBL"), ImVec2(thickness, thickness));
        if (ImGui::IsItemActive())
        {
            float new_w = m_WindowSize.x - mouse_delta.x;
            float new_h = m_WindowSize.y + mouse_delta.y;

            if (new_w >= min_w)
            {
                ImGui::SetWindowPos(ImVec2(win_pos.x + mouse_delta.x, win_pos.y));
                m_WindowSize.x = new_w;
            }
            m_WindowSize.y = ImMax(new_h, min_h);
        }
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);

        ImGui::SetCursorScreenPos(ImVec2(win_pos.x + win_size.x - thickness, win_pos.y));
        ImGui::InvisibleButton(xorstr_("##ResizeTR"), ImVec2(thickness, thickness));
        if (ImGui::IsItemActive())
        {
            float new_w = m_WindowSize.x + mouse_delta.x;
            float new_h = m_WindowSize.y - mouse_delta.y;

            m_WindowSize.x = ImMax(new_w, min_w);

            if (new_h >= min_h)
            {
                ImGui::SetWindowPos(ImVec2(win_pos.x, win_pos.y + mouse_delta.y));
                m_WindowSize.y = new_h;
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);

        ImGui::SetCursorScreenPos(ImVec2(win_pos.x, win_pos.y));
        ImGui::InvisibleButton(xorstr_("##ResizeTL"), ImVec2(thickness, thickness));
        if (ImGui::IsItemActive())
        {
            float new_w = m_WindowSize.x - mouse_delta.x;
            float new_h = m_WindowSize.y - mouse_delta.y;

            if (new_w >= min_w)
            {
                win_pos.x += mouse_delta.x;
                m_WindowSize.x = new_w;
            }
            if (new_h >= min_h)
            {
                win_pos.y += mouse_delta.y;
                m_WindowSize.y = new_h;
            }
            ImGui::SetWindowPos(win_pos);
        }

        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);

        if (chatSystem)
        {
            chatSystem->windowsX->value = win_pos.x;
            chatSystem->windowsY->value = win_pos.y;
            chatSystem->windowsSX->value = m_WindowSize.x;
            chatSystem->windowsSY->value = m_WindowSize.y;
        }

        ImGui::PopItemFlag();
    }

    static void DrawChatText(const char* text, float targetFontSize, float alpha, float wrapWidth)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImFont* font = ImGui::GetFont();
        ImVec2 pos = ImGui::GetCursorScreenPos();

        ImU32 mainColor = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, alpha));


        drawList->AddText(
            font,
            targetFontSize,
            pos,
            mainColor,
            text,
            nullptr,
            wrapWidth
        );

        ImVec2 textSize = font->CalcTextSizeA(targetFontSize, FLT_MAX, wrapWidth, text);

        ImGui::Dummy(textSize);
    }

    float EaseOutCubic(float t)
    {
        return 1.0f - powf(1.0f - t, 3.0f);
    }

public:
    MCChatSystem()
    {
        IsChatOpen = false;
        ScrollToBottom = false;
        JustOpened = false;
        ReclaimFocus = false;
        IsUserScroll = true;

        auto chatSystem = ModuleManager::Get().GetModule<ChatSystem>();

        if (chatSystem) m_WindowSize = ImVec2(chatSystem->windowsSX->value, chatSystem->windowsSY->value);
        else m_WindowSize = ImVec2(400, 300);

        memset(InputBuf, 0, sizeof(InputBuf));
    }

    void SetEnabled(bool enabled)
    {
        auto PC = Instances::GetPlayerController();
        UWB_Chat_C* ChatWidget = Instances::GetInstanceOf<UWB_Chat_C>();

        IsChatOpen = enabled;
        if (IsChatOpen)
        {
            ReclaimFocus = true;
            memset(InputBuf, 0, sizeof(InputBuf));
            JustOpened = true;
            if (ChatWidget)
            {
                ChatWidget->ChatMessage_InputBox->SetUserFocus(PC);
            }
        }
        else
        {
            if (ChatWidget)
            {
                UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
            }
        }
        ImGuiIO& io = ImGui::GetIO();


        if (PC)
        {
            if (IsChatOpen && PC->bShowMouseCursor == false)
            {
                io.MouseDrawCursor = true;
            }
            else if (!IsChatOpen)
            {
                io.MouseDrawCursor = false;
            }
        }
    }

    bool Toggle()
    {
        SetEnabled(IsChatOpen = !IsChatOpen);
        return IsChatOpen;
    }

    bool IsOpen() const { return IsChatOpen; }

    void AddLog(const char* fmt, ...) IM_FMTARGS(2) {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf) - 1] = 0;
        va_end(args);

        ChatMessage msg;
        msg.Text = std::string(buf);
        msg.Timestamp = (float)ImGui::GetTime();
        msg.Alpha = 1.0f;
        
        {
            std::lock_guard lock(m_MsgMutex);
            Messages.push_back(msg);
        }

        if (IsUserScroll)
        {
            ScrollToBottom = true;
        }
    }

    void OnUpdate()
    {
        auto chatSystem = ModuleManager::Get().GetModule<ChatSystem>();
        if (!chatSystem || chatSystem->IsDisabled()) return;

        auto PC = Instances::GetPlayerController();
        UWB_Chat_C* ChatWidget = Instances::GetInstanceOf<UWB_Chat_C>();

        if (ChatWidget)
        {
            if (PC)
            {
                if (IsChatOpen)
                {
                    if (!ChatWidget->ChatMessage_InputBox->HasKeyboardFocus())
                        ChatWidget->ChatMessage_InputBox->
                                    SetUserFocus(PC);
                }
                else
                {
                    if (ChatWidget->ChatMessage_InputBox->HasKeyboardFocus())
                        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
                }
            }

            FText t = ChatWidget->ChatMessage_InputBox->GetText();
            FString s = t.GetStringRef();
            std::string utf8 = Utils::WstringToUtf8(s.ToWString());

            strncpy_s(InputBuf, utf8.c_str(), sizeof(InputBuf) - 1);
            InputBuf[sizeof(InputBuf) - 1] = 0;
        }
    }

    void ResetChat()
    {
        FTickCallbackSystem::RegisterCallback([]
        {
            auto GameChatInstance = Instances::GetInstanceOf<UWB_Chat_C>();
            if (GameChatInstance)
            {
                GameChatInstance->SetRenderOpacity(1.0f);
            }
        });
    }

    void OnFunctionEvent(FunctionEvent* event)
    {
        auto chatSystem = ModuleManager::Get().GetModule<ChatSystem>();
        if (!chatSystem) return;

        std::string FuncName = event->functionName;

        if (FuncName == xorstr_("Function WB_Chat.WB_Chat_C.Tick"))
        {
            auto ChatWidget = (UWB_Chat_C*)event->context;
            
            ChatWidget->SetRenderOpacity(chatSystem->IsEnabled() ? chatSystem->oldChatOpacity->value : 1.0f);
        }

        if (!chatSystem->IsEnabled()) return;

        if (FuncName.find(xorstr_("WB_ChatMessage.WB_ChatMessage_C.Construct")) != std::string::npos)
        {
            auto Widget = (UWB_ChatMessage_C*)event->context;

            std::string Sender = Widget->ChatMessage.ChatSender_4_C7B4E1F24DE31402E441579224BD4102.ToString();
            std::string Content = Widget->ChatMessage.ChatMessage_5_75BB7DC5474425DE927E53B8D831BEA3.ToString();

            std::string FullMsg = xorstr_("[") + Sender + xorstr_("]: ") + Content;

            AddLog(FullMsg.c_str());
        }
    }

    void OnKeyEvent(KeyEvent* event)
    {
        auto chatSystem = ModuleManager::Get().GetModule<ChatSystem>();
        if (!chatSystem || chatSystem->IsDisabled()) return;

        if (event->type == Pressed && event->key == VK_RETURN && IsChatOpen)
        {
            event->cancel = true;

            auto GameChatInstance = Instances::GetInstanceOf<UWB_Chat_C>();

            auto World = Instances::GetWorld();
            if (!World) return;
            
            auto PC = Instances::GetPlayerController();
            
            std::string currentUtf8 = std::string(InputBuf);

            bool bIsCtrlDown = InputUtils::IsKeyDown(VK_LCONTROL, false);

            bool bShouldClose = chatSystem->autoClose->value && !bIsCtrlDown;

            if (!currentUtf8.empty())
            {
                if (GameChatInstance)
                {
                    std::wstring wideStr = Utils::Utf8ToWstring(currentUtf8);
                    
                    if (wideStr.length() > 0 && wideStr[0] == L'/')
                    {
                        std::wstring commandContent = wideStr.substr(1);
                        
                        FString MessageToCommand(commandContent.c_str());
    
                        if (PC) 
                        {
                            Instances::GetSystemLib()->ExecuteConsoleCommand(World, MessageToCommand, PC);
                        }
                        
                        std::string logMsg = xorstr_("Command: ") + Utils::WstringToUtf8(commandContent);
                        AddLog(logMsg.c_str());

                        if (!bIsCtrlDown)
                        {
                            if (GameChatInstance->ChatMessage_InputBox)
                            {
                                FString EmptyFString(L""); 
                                FText EmptyFText = Instances::GetTextLib()->Conv_StringToText(EmptyFString);
                                GameChatInstance->ChatMessage_InputBox->SetText(EmptyFText);
                            }
                        }
                    } else
                    {
                        FString MessageToSend(wideStr.c_str());
                    

                        FText LastText = GameChatInstance->ChatMessage_InputBox->GetText();

                        GameChatInstance->Send_Global_Message(MessageToSend);

                        if (bIsCtrlDown)
                        {
                            if (GameChatInstance->ChatMessage_InputBox)
                            {
                                GameChatInstance->ChatMessage_InputBox->SetText(LastText);
                            }
                        }
                    }
                }
            }

            if (bShouldClose)
            {
                SetEnabled(false);
            }
        }
    }
    void Draw()
    {
        auto chatSystem = ModuleManager::Get().GetModule<ChatSystem>();
        if (!chatSystem || chatSystem->IsDisabled()) return;

        ImGuiIO& io = ImGui::GetIO();
        ImGuiConfigFlags backup_flags = io.ConfigFlags;
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

        float fontScale = chatSystem->fontSize->value;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, chatSystem->windowBgColor->GetNU32());
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, chatSystem->windowRounding->value);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoResize;

        if (!IsChatOpen)
        {
            windowFlags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground;
        }
        ImVec2 defaultPos = ImVec2(chatSystem->windowsX->value, chatSystem->windowsY->value);

        ImGui::SetNextWindowPos(defaultPos, ImGuiCond_FirstUseEver);

        ImGui::SetNextWindowSize(m_WindowSize, ImGuiCond_Always);

        if (ImGui::Begin(xorstr_("MCChatOverlay"), nullptr, windowFlags))
        {
            ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);

            HandleWindowEdges();

            float currentTime = (float)ImGui::GetTime();
            float inputHeight = ImGui::GetFrameHeight();
            float windowHeight = ImGui::GetWindowHeight();

            float inputBoxY = windowHeight - inputHeight - chatSystem->inputBottomOffset->value;
            float logRegionHeight = inputBoxY - chatSystem->inputPadding->value;

            float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
            float fixedWrapWidth = m_WindowSize.x - (chatSystem->horizontalPadding->value * 2) - scrollbarWidth;

            ImGuiWindowFlags childFlags = ImGuiWindowFlags_None;
            if (!IsChatOpen)
            {
                childFlags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoInputs;
            }

            ImGui::SetCursorPos(ImVec2(chatSystem->horizontalPadding->value, 0));

            if (JustOpened)
            {
                ImGui::SetNextWindowScroll(ImVec2(0.0f, FLT_MAX));
            }

            ImGui::BeginChild(xorstr_("LogRegion"),
                              ImVec2(m_WindowSize.x - chatSystem->horizontalPadding->value * 2, logRegionHeight), false,
                              childFlags);
            
            {
                std::lock_guard lock(m_MsgMutex);

                if (chatSystem->renderFromBottom->value)
                {
                    float total_content_height = 0.0f;
                    for (auto& msg : Messages)
                    {
                        ImVec2 sz = ImGui::CalcTextSize2(msg.Text.c_str(), fontScale, nullptr, false, fixedWrapWidth);
                        total_content_height += sz.y + ImGui::GetStyle().ItemSpacing.y;
                    }
                    if (total_content_height > 0) total_content_height -= ImGui::GetStyle().ItemSpacing.y;

                    if (total_content_height < logRegionHeight)
                    {
                        ImGui::SetCursorPosY(logRegionHeight - total_content_height);
                        ImGui::Dummy(ImVec2(0.0f, 0.0f));
                    }
                }

                float FadeDuration = chatSystem->fadeDuration->value;
                float MessageLifeTime = chatSystem->messageLifeTime->value;

                for (auto& msg : Messages)
                {
                    float timeAlive = currentTime - msg.Timestamp;

                    if (IsChatOpen)
                    {
                        msg.Alpha = 1.0f;
                    }
                    else
                    {
                        if (timeAlive > MessageLifeTime + FadeDuration) msg.Alpha = 0.0f;
                        else if (timeAlive > MessageLifeTime)
                            msg.Alpha = 1.0f - (timeAlive - MessageLifeTime) /
                            FadeDuration;
                        else msg.Alpha = 1.0f;
                    }


                    float entryAlpha = 1.0f;
                    float xOffset = 0.0f;

                    if (timeAlive < chatSystem->inAnimationDuration->value)
                    {
                        float t = timeAlive / chatSystem->inAnimationDuration->value;

                        float ease = EaseOutCubic(t);

                        if (chatSystem->enableFadeIn->value)
                        {
                            entryAlpha = ease;
                        }

                        if (chatSystem->enableSlideIn->value)
                        {
                            xOffset = -150.0f * (1.0f - ease);
                        }
                    }


                    float finalAlpha = msg.Alpha * entryAlpha;

                    float wipeProgress = 1.0f - msg.Alpha;

                    if (msg.Alpha > 0.01f)
                    {
                        ImVec2 originalPos = ImGui::GetCursorPos();
                        ImGui::SetCursorPosX(originalPos.x + xOffset);

                        if (chatSystem->useWipeAnimation->value && !IsChatOpen && wipeProgress > 0.0f)
                        {
                            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                            ImVec2 textSize = ImGui::CalcTextSize2(msg.Text.c_str(), fontScale, nullptr, false,
                                                                   fixedWrapWidth);

                            float clipTopY = cursorPos.y + (textSize.y * wipeProgress);
                            float clipBottomY = cursorPos.y + textSize.y + 2.0f;

                            ImGui::PushClipRect(ImVec2(cursorPos.x - 2, clipTopY),
                                                ImVec2(cursorPos.x + fixedWrapWidth + 4, clipBottomY), true);


                            DrawChatText(msg.Text.c_str(), fontScale, 1.0f, fixedWrapWidth);

                            ImGui::PopClipRect();
                        }
                        else
                        {
                            DrawChatText(msg.Text.c_str(), fontScale, finalAlpha, fixedWrapWidth);
                        }
                    }
                    else
                    {
                        if (chatSystem->renderFromBottom->value)
                        {
                            ImVec2 textSize = ImGui::CalcTextSize2(msg.Text.c_str(), fontScale, nullptr, false,
                                                                   fixedWrapWidth);
                            ImGui::Dummy(textSize);
                        }
                    }
                }
            }

            if (ImGui::GetScrollMaxY() > 0)
            {
                IsUserScroll = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f;
            }
            else
            {
                IsUserScroll = true;
            }

            if (ScrollToBottom)
            {
                ImGui::SetScrollHereY(1.0f);
                ScrollToBottom = false;
            }
            if (JustOpened)
            {
                ImGui::SetScrollHereY(1.0f);
                JustOpened = false;
            }
            if (!IsChatOpen && chatSystem->renderFromBottom->value) { ImGui::SetScrollHereY(1.0f); }

            ImGui::EndChild();

            if (IsChatOpen)
            {
                float padX = chatSystem->horizontalPadding->value;
                float boxW = m_WindowSize.x - padX * 2.0f;
                float boxH = ImGui::GetFrameHeight();
                
                float innerPadding = 6.0f;

                ImVec2 winPos = ImGui::GetWindowPos();

                ImVec2 boxMin(winPos.x + padX, winPos.y + inputBoxY);
                ImVec2 boxMax(boxMin.x + boxW, boxMin.y + boxH);

                ImDrawList* dl = ImGui::GetWindowDrawList();
                
                dl->AddRectFilled(boxMin, boxMax, chatSystem->inputBgColor->GetNU32(),
                                  chatSystem->inputRounding->value);
                
                ImVec2 textSize = ImGui::CalcTextSize(InputBuf);
                
                float availableWidth = boxW - (innerPadding * 2.0f);
                
                float scrollOffset = 0.0f;
                if (textSize.x > availableWidth)
                {
                    scrollOffset = textSize.x - availableWidth;
                    scrollOffset += 5.0f;
                }
                
                ImVec2 textPos(boxMin.x + innerPadding - scrollOffset,
                               boxMin.y + (boxH - ImGui::GetFontSize()) * 0.5f);
                
                dl->PushClipRect(boxMin, boxMax, true);

               
                dl->AddText(textPos, IM_COL32(255, 255, 255, 255), InputBuf);

           
                int32_t memCursorIndex = -1; 

              
                int bufferLen = (int)strlen(InputBuf);

                if (memCursorIndex == -1) {
                    memCursorIndex = bufferLen; 
                }

              
                if (memCursorIndex < 0) memCursorIndex = 0;
                if (memCursorIndex > bufferLen) memCursorIndex = bufferLen;

                const bool blinkOn = fmodf((float)ImGui::GetTime(), 1.0f) < 0.5f;
                if (blinkOn)
                {
                    ImVec2 sizeBeforeCaret = ImGui::CalcTextSize(InputBuf, InputBuf + memCursorIndex);
                    
                    float caretX = textPos.x + sizeBeforeCaret.x; 
                    
                    if (memCursorIndex > 0) {
                        caretX += 1.0f; 
                    }

                    float caretTop = boxMin.y + 4.0f;
                    float caretBot = boxMax.y - 4.0f;

                    dl->AddLine(ImVec2(caretX, caretTop), ImVec2(caretX, caretBot),
                                IM_COL32(255, 255, 255, 220), 1.5f);
                }
    
                dl->PopClipRect();
            }


            ImGui::PopItemFlag();
        }
        ImGui::End();

        ImGui::PopStyleVar(1);
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(1);

        io.ConfigFlags = backup_flags;
    }
};