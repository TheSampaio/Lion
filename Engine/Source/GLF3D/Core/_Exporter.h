#ifndef GLF3D_EXPORTER_H
#define GLF3D_EXPORTER_H

#ifdef GLF_WIN
	#ifdef GLF_DLL
		#define GLF3D_API __declspec(dllexport)
	#else
		#define GLF3D_API __declspec(dllimport)
	#endif
#else
	#ifdef GLF_DLL
		#define GLF3D_API __attribute__((visibility("default")))
	#else
		#define GLF3D_API
	#endif
#endif

#endif // !GLF3D_EXPORTER_H
