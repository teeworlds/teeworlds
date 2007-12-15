/* Include the auto configuration */
#include <engine/e_detect.h>

/* common part */
#include "portaudio/pa_converters.c"
#include "portaudio/pa_trace.c"
#include "portaudio/pa_front.c"
#include "portaudio/pa_dither.c"
#include "portaudio/pa_process.c"
#include "portaudio/pa_stream.c"
#include "portaudio/pa_cpuload.c"
#include "portaudio/pa_allocation.c"
#include "portaudio/pa_debugprint.c"

#if defined(CONF_FAMILY_UNIX)
	#include "portaudio/pa_unix_util.c"
	#if defined(CONF_PLATFORM_MACOSX)
		#define PA_USE_COREAUDIO
		#include "portaudio/pa_mac_core.c"
		#include "portaudio/pa_mac_core_blocking.c"
		#include "portaudio/pa_mac_core_utilities.c"
		#include "portaudio/pa_mac_hostapis.c"
		#include "portaudio/pa_ringbuffer.c"
	#else
		#define HAVE_SYS_SOUNDCARD_H
		#define PA_USE_ALSA
		#include "portaudio/pa_linux_alsa.c"
		//#include "portaudio/pa_unix_oss.c"
		#include "portaudio/pa_unix_hostapis.c"
	#endif
#elif defined(CONF_FAMILY_WINDOWS)
	#define PA_NO_WMME
	#define PA_NO_ASIO
	#include "portaudio/pa_win_ds.c"
	#include "portaudio/pa_win_ds_dynlink.c"
	#include "portaudio/pa_win_hostapis.c"
	#include "portaudio/pa_win_util.c"
#else
	#error help me!
#endif
