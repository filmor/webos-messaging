/* ============================================================
 * Date  : Apr 15, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJAPP_H_
#define MOJAPP_H_

#include "core/MojCoreDefs.h"
#include "core/MojAutoPtr.h"
#include "core/MojMap.h"
#include "core/MojObject.h"
#include "core/MojString.h"
#include "core/MojVector.h"

class MojApp : private MojNoCopy
{
public:
	static const MojUInt32 MajorVersion = 1;
	static const MojUInt32 MinorVersion = 0;

	virtual ~MojApp();
	const MojString& name() const { return m_name; }

	virtual int main(int argc, MojChar** argv);
	virtual MojErr configure(const MojObject& conf);
	virtual MojErr init();
	virtual MojErr open();
	virtual MojErr close();
	virtual MojErr run();

protected:
	typedef MojErr (MojApp::*OptionHandler)(const MojString& name, const MojString& val);
	typedef MojVector<MojString> StringVec;

	MojApp(MojUInt32 majorVersion = MajorVersion, MojUInt32 minorVersion = MinorVersion, const MojChar* versionString = NULL);
	MojErr registerOption(OptionHandler handler, const MojChar* opt, const MojChar* help, bool argRequired = false);

	virtual MojErr displayMessage(const MojChar* format, ...) MOJ_FORMAT_ATTR((printf, 2, 3));
	virtual MojErr displayErr(const MojChar* prefix, const MojChar* errToDisplay);
	virtual MojErr displayErr(MojErr errToDisplay, const MojChar* message);
	virtual MojErr displayLoggers();
	virtual MojErr displayUsage();
	virtual MojErr displayOptions();
	virtual MojErr displayVersion();
	virtual MojErr handleCommandLine(int argc, MojChar** argv);
	virtual MojErr handleArgs(const StringVec& args);

protected:
	static MojLogger s_log;

private:
	enum RunMode {
		ModeDefault,
		ModeHelp,
		ModeLoggers,
		ModeVersion
	};
	struct Option {
		Option(OptionHandler handler, const MojString& help, bool argRequired)
		: m_handler(handler), m_help(help), m_arg(argRequired) {}
		bool operator<(const Option& rhs) const { return m_help < rhs.m_help; }

		OptionHandler m_handler;
		MojString m_help;
		bool m_arg;
	};
	typedef MojMap<MojString, Option, const MojChar*> OptHandlerMap;

	MojErr handleConfig(const MojString& name, const MojString& val);
	MojErr handleHelp(const MojString& name, const MojString& val);
	MojErr handleLoggers(const MojString& name, const MojString& val);
	MojErr handleVersion(const MojString& name, const MojString& val);
	MojErr loadConfig(const MojChar* path);

	MojString m_name;
	OptHandlerMap m_optMap;
	MojObject m_conf;
	RunMode m_runMode;
	bool m_errDisplayed;
	MojUInt32 m_majorVersion;
	MojUInt32 m_minorVersion;
	const MojChar* m_versionString;
};

#endif /* MOJAPP_H_ */
