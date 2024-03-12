#pragma once

namespace MSGPack
{
	#ifdef _DEBUG
		static constexpr const bool SecureBase = true;
	#else
		static constexpr const bool SecureBase = false;
	#endif
}
