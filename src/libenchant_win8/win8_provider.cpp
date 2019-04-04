/* HexChat
 * Copyright (c) 2015 Patrick Griffis
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

#include "config.h"

#include <Spellcheck.h>
#include <glib.h>

#include "typedef.h" // for ssize_t
#include <enchant-provider.h>

ENCHANT_PLUGIN_DECLARE ("win8")

/* --------- Utils ----------*/

static char *
utf16_to_utf8 (const wchar_t * const str, gboolean from_bcp47)
{
	char *utf8 = g_utf16_to_utf8 ((gunichar2*)str, -1, nullptr, nullptr, nullptr);
	if (utf8 && from_bcp47)
	{
		char *p = utf8;
		/* bcp47 tags use syntax "en-US" while the myspell versions are "en_US" */
		while (*p)
		{
			if (*p == '-')
				*p = '_';
			p++;
		}
	}
	return utf8;
}

static wchar_t *
utf8_to_utf16 (const char * const str, size_t len, gboolean to_bcp47)
{
	wchar_t *utf16 = (wchar_t*)g_utf8_to_utf16 (str, len, nullptr, nullptr, nullptr);
	if (utf16 && to_bcp47)
	{
		wchar_t *p = utf16;
		/* bcp47 tags use syntax "en-US" while the myspell versions are "en_US" */
		while (*p)
		{
			if (*p == L'_')
				*p = L'-';
			p++;
		}
	}
	return utf16;
}

static char **
enumstring_to_chararray (IEnumString *strings, size_t *out_len, gboolean from_bcp47)
{
	char **chars = g_new (char*, 256); /* Hopefully large enough */
	LPOLESTR wstr = nullptr;
	size_t i = 0;

	while (SUCCEEDED (strings->Next (1, &wstr, nullptr)) && i < 256 && wstr)
	{
		char *str = utf16_to_utf8 (wstr, from_bcp47);
		if (str)
		{
			chars[i] = str;
			i++;
		}
		CoTaskMemFree (wstr);
	}
	chars[i] = nullptr;
	strings->Release ();

	*out_len = i;
	return chars;
}

/* ---------- Dict ------------ */

static void
win8_dict_add_to_personal (EnchantDict *dict, const char *const word, size_t len)
{
	auto checker = static_cast<ISpellChecker*>(dict->user_data);
	wchar_t *wword = utf8_to_utf16 (word, len, FALSE);

	checker->Add (wword);
	g_free (wword);
}

static void
win8_dict_add_to_session (EnchantDict *dict, const char *const word, size_t len)
{
	auto checker = static_cast<ISpellChecker*>(dict->user_data);
	wchar_t *wword = utf8_to_utf16 (word, len, FALSE);

	checker->Ignore (wword);
	g_free (wword);
}

static int
win8_dict_check (EnchantDict *dict, const char *const word, size_t len)
{
	auto checker = static_cast<ISpellChecker*>(dict->user_data);
	wchar_t *wword = utf8_to_utf16 (word, len, FALSE);
	IEnumSpellingError *errors;
	ISpellingError *error = nullptr;
	HRESULT hr;

	hr = checker->Check (wword, &errors);
	g_free (wword);

	if (FAILED (hr))
		return -1; /* Error */

	if (errors->Next (&error) == S_OK)
	{
		error->Release ();
		errors->Release ();
		return 1; /* Spelling Issue */
	}
	else
	{
		errors->Release ();
		return 0; /* Correct */
	}
}

static char **
win8_dict_suggest (EnchantDict *dict, const char *const word, size_t len, size_t *out_n_suggs)
{
	auto checker = static_cast<ISpellChecker*>(dict->user_data);
	wchar_t *wword = utf8_to_utf16 (word, len, FALSE);
	IEnumString *suggestions;
	HRESULT hr;

	hr = checker->Suggest (wword, &suggestions);
	g_free (wword);

	if (FAILED (hr))
	{
		*out_n_suggs = 0;
		return nullptr;
	}

	return enumstring_to_chararray (suggestions, out_n_suggs, FALSE);
}

/* ---------- Provider ------------ */

static EnchantDict *
win8_provider_request_dict (EnchantProvider *provider, const char *const tag)
{
	auto factory = static_cast<ISpellCheckerFactory*>(provider->user_data);
	ISpellChecker *checker;
	EnchantDict *dict;
	wchar_t *wtag = utf8_to_utf16 (tag, -1, TRUE);
	HRESULT hr;

	hr = factory->CreateSpellChecker (wtag, &checker);
	g_free (wtag);

	if (FAILED (hr))
		return nullptr;

	dict = g_new0 (EnchantDict, 1);
	dict->suggest = win8_dict_suggest;
	dict->check = win8_dict_check;
	dict->add_to_personal = win8_dict_add_to_personal;
	dict->add_to_exclude = win8_dict_add_to_personal; /* Basically the same */
	dict->add_to_session = win8_dict_add_to_session;

	dict->user_data = checker;

	return dict;
}

static void
win8_provider_dispose_dict (EnchantProvider *provider, EnchantDict *dict)
{
	if (dict)
	{
		auto checker = static_cast<ISpellChecker*>(dict->user_data);

		checker->Release ();
		g_free (dict);
	}
}

static int
win8_provider_dictionary_exists (EnchantProvider *provider, const char *const tag)
{
	auto factory = static_cast<ISpellCheckerFactory*>(provider->user_data);
	wchar_t *wtag = utf8_to_utf16 (tag, -1, TRUE);

	BOOL is_supported = FALSE;
	factory->IsSupported (wtag, &is_supported);

	g_free (wtag);
	return is_supported;
}


static char **
win8_provider_list_dicts (EnchantProvider *provider, size_t *out_n_dicts)
{
	auto factory = static_cast<ISpellCheckerFactory*>(provider->user_data);
	IEnumString *dicts;

	if (FAILED(factory->get_SupportedLanguages (&dicts)))
	{
		*out_n_dicts = 0;
		return nullptr;
	}

	return enumstring_to_chararray (dicts, out_n_dicts, TRUE);
}

static void
win8_provider_free_string_list (EnchantProvider *provider, char **str_list)
{
	g_strfreev (str_list);
}

static void
win8_provider_dispose (EnchantProvider *provider)
{
	if (provider)
	{
		auto factory = static_cast<ISpellCheckerFactory*>(provider->user_data);

		factory->Release();
		g_free (provider);
	}
}

static const char *
win8_provider_identify (EnchantProvider *provider)
{
	return "win8";
}

static const char *
win8_provider_describe (EnchantProvider *provider)
{
	return "Windows 8 SpellCheck Provider";
}

extern "C"
{

EnchantProvider *
init_enchant_provider (void)
{
	EnchantProvider *provider;
	ISpellCheckerFactory *factory;

	if (FAILED (CoCreateInstance (__uuidof(SpellCheckerFactory), nullptr,
				CLSCTX_INPROC_SERVER, IID_PPV_ARGS (&factory))))
		return nullptr;

	provider = g_new0 (EnchantProvider, 1);
	provider->dispose = win8_provider_dispose;
	provider->request_dict = win8_provider_request_dict;
	provider->dispose_dict = win8_provider_dispose_dict;
	provider->dictionary_exists = win8_provider_dictionary_exists;
	provider->identify = win8_provider_identify;
	provider->describe = win8_provider_describe;
	provider->list_dicts = win8_provider_list_dicts;
	provider->free_string_list = win8_provider_free_string_list;

	provider->user_data = factory;

	return provider;
}

}
