#ifndef JPLUGIN_H
#define JPLUGIN_H

#include <qutim/plugin.h>

namespace Jabber {

	using namespace qutim_sdk_0_3;

	class jPlugin : public Plugin
	{
		Q_OBJECT

		public:
			jPlugin();
			void init();
			bool load();
			bool unload();
	};
} // Jabber namespace

#endif // JPLUGIN_H
