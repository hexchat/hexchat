@echo off

echo.Compiling translations . . .
cd ..\po
rmdir /Q /S locale
mkdir locale

mkdir locale\be\LC_MESSAGES
msgfmt -cvo locale\be\LC_MESSAGES\xchat.mo be.po

mkdir locale\ca\LC_MESSAGES
msgfmt -cvo locale\ca\LC_MESSAGES\xchat.mo ca.po

mkdir locale\cs\LC_MESSAGES
msgfmt -cvo locale\cs\LC_MESSAGES\xchat.mo cs.po

mkdir locale\de\LC_MESSAGES
msgfmt -cvo locale\de\LC_MESSAGES\xchat.mo de.po

mkdir locale\el\LC_MESSAGES
msgfmt -cvo locale\el\LC_MESSAGES\xchat.mo el.po

mkdir locale\es\LC_MESSAGES
msgfmt -cvo locale\es\LC_MESSAGES\xchat.mo es.po

mkdir locale\fi\LC_MESSAGES
msgfmt -cvo locale\fi\LC_MESSAGES\xchat.mo fi.po

mkdir locale\fr\LC_MESSAGES
msgfmt -cvo locale\fr\LC_MESSAGES\xchat.mo fr.po

mkdir locale\gl\LC_MESSAGES
msgfmt -cvo locale\gl\LC_MESSAGES\xchat.mo gl.po

mkdir locale\hu\LC_MESSAGES
msgfmt -cvo locale\hu\LC_MESSAGES\xchat.mo hu.po

mkdir locale\it\LC_MESSAGES
msgfmt -cvo locale\it\LC_MESSAGES\xchat.mo it.po

mkdir locale\ja\LC_MESSAGES
msgfmt -cvo locale\ja\LC_MESSAGES\xchat.mo ja.po

mkdir locale\ko\LC_MESSAGES
msgfmt -cvo locale\ko\LC_MESSAGES\xchat.mo ko.po

mkdir locale\lt\LC_MESSAGES
msgfmt -cvo locale\lt\LC_MESSAGES\xchat.mo lt.po

mkdir locale\mk\LC_MESSAGES
msgfmt -cvo locale\mk\LC_MESSAGES\xchat.mo mk.po

mkdir locale\nl\LC_MESSAGES
msgfmt -cvo locale\nl\LC_MESSAGES\xchat.mo nl.po

mkdir locale\pa\LC_MESSAGES
msgfmt -cvo locale\pa\LC_MESSAGES\xchat.mo pa.po

mkdir locale\pt\LC_MESSAGES
msgfmt -cvo locale\pt\LC_MESSAGES\xchat.mo pt.po

mkdir locale\ru\LC_MESSAGES
msgfmt -cvo locale\ru\LC_MESSAGES\xchat.mo ru.po

mkdir locale\sq\LC_MESSAGES
msgfmt -cvo locale\sq\LC_MESSAGES\xchat.mo sq.po

mkdir locale\sr\LC_MESSAGES
msgfmt -cvo locale\sr\LC_MESSAGES\xchat.mo sr.po

mkdir locale\sv\LC_MESSAGES
msgfmt -cvo locale\sv\LC_MESSAGES\xchat.mo sv.po

mkdir locale\th\LC_MESSAGES
msgfmt -cvo locale\th\LC_MESSAGES\xchat.mo th.po

mkdir locale\uk\LC_MESSAGES
msgfmt -cvo locale\uk\LC_MESSAGES\xchat.mo uk.po

mkdir locale\vi\LC_MESSAGES
msgfmt -cvo locale\vi\LC_MESSAGES\xchat.mo vi.po

mkdir locale\zh_CN\LC_MESSAGES
msgfmt -cvo locale\zh_CN\LC_MESSAGES\xchat.mo zh_CN.po

mkdir locale\zh_TW\LC_MESSAGES
msgfmt -cvo locale\zh_TW\LC_MESSAGES\xchat.mo zh_TW.po
