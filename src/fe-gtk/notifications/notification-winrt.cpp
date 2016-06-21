/* HexChat
 * Copyright (c) 2014 Leetsoftwerx
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string>
#include <codecvt>

#include <roapi.h>
#include <windows.ui.notifications.h>

using namespace Windows::UI::Notifications;
using namespace Windows::Data::Xml::Dom;

static ToastNotifier ^ notifier = nullptr;

static std::wstring
widen(const std::string & to_widen)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
	return converter.from_bytes(to_widen);
}

extern "C"
{
	__declspec (dllexport) void
	notification_backend_show (const char *title, const char *text)
	{
		try
		{
			auto toastTemplate = ToastNotificationManager::GetTemplateContent (ToastTemplateType::ToastText02);
			auto node_list = toastTemplate->GetElementsByTagName (L"text");
			UINT node_count = node_list->Length;

			auto wtitle = widen (title);
			node_list->GetAt (0)->AppendChild (
				toastTemplate->CreateTextNode (Platform::StringReference (wtitle.c_str (), wtitle.size ())));

			auto wtext = widen (text);
			node_list->GetAt (1)->AppendChild (
				toastTemplate->CreateTextNode (Platform::StringReference (wtext.c_str (), wtext.size ())));

			// Mute sound, we already play our own
			auto node = toastTemplate->SelectSingleNode (L"/toast");
			auto audio_elem = toastTemplate->CreateElement (L"audio");
			audio_elem->SetAttribute (L"silent", L"true");
			static_cast<XmlElement^>(node)->AppendChild (audio_elem);

			notifier->Show (ref new ToastNotification (toastTemplate));
		}
		catch (Platform::Exception ^ ex)
		{
		}
		catch (...)
		{
		}
	}

	__declspec (dllexport) int
	notification_backend_init (void)
	{
		if (!notifier)
			notifier = ToastNotificationManager::CreateToastNotifier (L"HexChat.Desktop.Notify");

		if (FAILED (Windows::Foundation::Initialize (RO_INIT_SINGLETHREADED)))
			return 0;

		return 1;
	}

	__declspec (dllexport) 	void
	notification_backend_deinit (void)
	{
		notifier = nullptr;
		Windows::Foundation::Uninitialize ();
	}

	__declspec (dllexport) int
	notification_backend_supported (void)
	{
		return 1;
	}
}
