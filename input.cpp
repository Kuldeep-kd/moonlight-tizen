#include "moonlight.hpp"

#include "ppapi/c/ppb_input_event.h"

#include "ppapi/cpp/input_event.h"

#include <Limelight.h>

#define KEY_PREFIX 0x80

#define KEY_CODE_ALT 18
#define KEY_CODE_CTRL 17
#define KEY_CODE_SHIFT 16

static int ConvertPPButtonToLiButton(PP_InputEvent_MouseButton ppButton) {
    switch (ppButton) {
        case PP_INPUTEVENT_MOUSEBUTTON_LEFT:
            return BUTTON_LEFT;
        case PP_INPUTEVENT_MOUSEBUTTON_MIDDLE:
            return BUTTON_MIDDLE;
        case PP_INPUTEVENT_MOUSEBUTTON_RIGHT:
            return BUTTON_RIGHT;
        default:
            return 0;
    }
}

void MoonlightInstance::DidLockMouse(int32_t result) {
    m_MouseLocked = (result == PP_OK);
}

void MoonlightInstance::MouseLockLost() {
    m_MouseLocked = false;
    m_KeyModifiers = 0;
}

void MoonlightInstance::UpdateModifiers(PP_InputEvent_Type eventType, short keyCode) {
    switch (keyCode) {
        case KEY_CODE_ALT:
            if (eventType == PP_INPUTEVENT_TYPE_KEYDOWN) {
                m_KeyModifiers |= MODIFIER_ALT;
            }
            else {
                m_KeyModifiers &= ~MODIFIER_ALT;
            }
            break;
            
        case KEY_CODE_CTRL:
            if (eventType == PP_INPUTEVENT_TYPE_KEYDOWN) {
                m_KeyModifiers |= MODIFIER_CTRL;
            }
            else {
                m_KeyModifiers &= ~MODIFIER_CTRL;
            }
            break;
            
        case KEY_CODE_SHIFT:
            if (eventType == PP_INPUTEVENT_TYPE_KEYDOWN) {
                m_KeyModifiers |= MODIFIER_SHIFT;
            }
            else {
                m_KeyModifiers &= ~MODIFIER_SHIFT;
            }
            break;
    }
}

bool MoonlightInstance::HandleInputEvent(const pp::InputEvent& event) {
    switch (event.GetType()) {
        case PP_INPUTEVENT_TYPE_MOUSEDOWN: {
            // Lock the mouse cursor when the user clicks on the stream
            if (!m_MouseLocked) {
                g_Instance->LockMouse(g_Instance->m_CallbackFactory.NewCallback(&MoonlightInstance::DidLockMouse));
                
                // Assume it worked until we get a callback telling us otherwise
                m_MouseLocked = true;
                return true;
            }
            
            pp::MouseInputEvent mouseEvent(event);
            
            LiSendMouseButtonEvent(BUTTON_ACTION_PRESS, ConvertPPButtonToLiButton(mouseEvent.GetButton()));
            return true;
        }
        
        case PP_INPUTEVENT_TYPE_MOUSEMOVE: {
            if (!m_MouseLocked) {
                return false;
            }
            
            pp::MouseInputEvent mouseEvent(event);
            pp::Point posDelta = mouseEvent.GetMovement();
            
            LiSendMouseMoveEvent(posDelta.x(), posDelta.y());
            return true;
        }
        
        case PP_INPUTEVENT_TYPE_MOUSEUP: {
            if (!m_MouseLocked) {
                return false;
            }
            
            pp::MouseInputEvent mouseEvent(event);
            
            LiSendMouseButtonEvent(BUTTON_ACTION_RELEASE, ConvertPPButtonToLiButton(mouseEvent.GetButton()));
            return true;
        }
        
        case PP_INPUTEVENT_TYPE_WHEEL: {
            if (!m_MouseLocked) {
                return false;
            }
            
            pp::WheelInputEvent wheelEvent(event);
            
            // FIXME: Handle fractional scroll ticks
            LiSendScrollEvent((signed char) wheelEvent.GetTicks().y());
            return true;
        }
        
        case PP_INPUTEVENT_TYPE_KEYDOWN: {
            if (!m_MouseLocked) {
                return false;
            }
            
            pp::KeyboardInputEvent keyboardEvent(event);
            
            // Update modifier state before sending the key event
            UpdateModifiers(event.GetType(), keyboardEvent.GetKeyCode());
            
            if (m_KeyModifiers == (MODIFIER_ALT | MODIFIER_CTRL | MODIFIER_SHIFT)) {
                g_Instance->UnlockMouse();
                m_MouseLocked = false;
                return true;
            }
            
            LiSendKeyboardEvent(KEY_PREFIX << 8 | keyboardEvent.GetKeyCode(),
                                KEY_ACTION_DOWN, m_KeyModifiers);
            return true;
        }
        
         case PP_INPUTEVENT_TYPE_KEYUP: {
            if (!m_MouseLocked) {
                return false;
            }
            
            pp::KeyboardInputEvent keyboardEvent(event);
            
            // Update modifier state before sending the key event
            UpdateModifiers(event.GetType(), keyboardEvent.GetKeyCode());
            
            LiSendKeyboardEvent(KEY_PREFIX << 8 | keyboardEvent.GetKeyCode(),
                                KEY_ACTION_UP, m_KeyModifiers);
            return true;
        }
        
        default: {
            return false;
        }
    }
}