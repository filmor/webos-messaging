/* ============================================================
 * Date  : Oct 1, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJLOGENGINE_H_
#define MOJLOGENGINE_H_

#include "core/MojCoreDefs.h"
#include "core/MojList.h"
#include "core/MojMap.h"
#include "core/MojSet.h"
#include "core/MojString.h"
#include "core/MojThread.h"

class MojLogAppender : private MojNoCopy
{
public:
	virtual ~MojLogAppender() {}
	virtual MojErr append(MojLogger::Level level, MojLogger* logger, const MojChar* format, va_list args) = 0;
};

class MojFileAppender : public MojLogAppender
{
public:
	MojFileAppender();
	~MojFileAppender();

	void open(MojFileT file);
	MojErr open(const MojChar* path);
	MojErr close();

	virtual MojErr append(MojLogger::Level level, MojLogger* logger, const MojChar* format, va_list args);

private:
	bool m_close;
	MojFileT m_file;
	MojString m_buf;
};

class MojSyslogAppender : public MojLogAppender
{
public:
	MojSyslogAppender();
	~MojSyslogAppender();

	void open(const MojChar* name);
	void close();
	virtual MojErr append(MojLogger::Level level, MojLogger* logger, const MojChar* format, va_list args);

private:
	MojString m_buf;
};

class MojLogEngine : private MojNoCopy
{
public:
	static const MojChar* const AppenderKey;
	static const MojChar* const DefaultKey;
	static const MojChar* const ErrorKey;
	static const MojChar* const LevelsKey;
	static const MojChar* const LogKey;
	static const MojChar* const PathKey;
	static const MojChar* const TypeKey;

	typedef MojSet<MojString> StringSet;

	MojLogEngine();
	~MojLogEngine();

	static MojLogEngine* instance();

	MojLogger* defaultLogger() { return &m_defaultLogger; }
	MojLogger* errorLogger() { return &m_errorLogger; }
	MojErr loggerNames(StringSet& setOut);

	MojErr configure(const MojObject& conf, const MojChar* name);
	void reset(const MojLogger::Level level);
	void appender(MojAutoPtr<MojLogAppender> appender);
	void log(MojLogger::Level level, MojLogger* logger, const MojChar* format, va_list args);

private:
	friend class MojLogger;
	typedef MojList<MojLogger, &MojLogger::m_entry> LoggerList;
	typedef MojMap<MojString, MojLogger::Level> LevelMap;

	void addLogger(MojLogger* logger);
	void removeLogger(MojLogger* logger);
	void updateAppender(MojAutoPtr<MojLogAppender> appender);
	void updateLoggers();
	void updateLoggerLevel(MojLogger* logger);
	MojErr createAppender(const MojObject& conf, const MojChar* name, MojAutoPtr<MojLogAppender>& appenderOut);

	MojThreadMutex m_mutex;
	LoggerList m_loggers;
	LevelMap m_levels;
	MojString m_name;
	MojLogger m_defaultLogger;
	MojLogger m_errorLogger;
	MojAutoPtr<MojLogAppender> m_configuredAppender;
	MojFileAppender m_defaultAppender;
	MojLogAppender* m_appender;
	MojThreadIdT m_logThread;
};

#endif /* MOJLOGENGINE_H_ */
