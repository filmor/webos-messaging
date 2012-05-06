/*
 * IMAccountValidatorApp.h
 *
 * Copyright 2010 Palm, Inc. All rights reserved.
 *
 * This program is free software and licensed under the terms of the GNU
 * General Public License Version 2 as published by the Free
 * Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-
 * 1301, USA
 *
 * IMAccountValidator uses libpurple.so to implement the checking of credentials when logging into an IM account
 */

#ifndef IMACCOUNTVALIDATORAPP_H_
#define IMACCOUNTVALIDATORAPP_H_

#include "core/MojGmainReactor.h"
#include "core/MojReactorApp.h"
#include "core/MojMessageDispatcher.h"
#include "luna/MojLunaService.h"
#include "IMAccountValidatorHandler.h"

class IMAccountValidatorApp : public MojReactorApp<MojGmainReactor>
{
public:
	static const char* const ServiceName;
	static MojLogger s_log;

	IMAccountValidatorApp();
	MojErr initializeLibPurple();

	virtual MojErr open();
	virtual MojErr close();

private:


	typedef MojReactorApp<MojGmainReactor> Base;

    MojMessageDispatcher m_dispatcher;
	MojRefCountedPtr<IMAccountValidatorHandler> m_handler;
	MojLunaService  m_service;
};

#endif /* IMACCOUNTVALIDATORAPP_H_ */
