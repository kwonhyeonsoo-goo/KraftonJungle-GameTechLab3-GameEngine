#pragma once

#pragma region Singleton

#define NO_COPY(ClassName)									\
		ClassName(const ClassName&) = delete; 				\
		ClassName(ClassName&&) = delete;					\
		ClassName& operator=(const ClassName&) = delete; 	\
		ClassName& operator=(ClassName&&) = delete; 		

#define DECLARE_SINGLETON(ClassName)						\
		NO_COPY(ClassName)									\
		public:												\
		static ClassName& GetInstance();


#define IMPLEMENT_SINGLETON(ClassName)						\
		ClassName& ClassName::GetInstance()					\
		{													\
			static ClassName sClass;						\
			return sClass;									\
		}

#pragma endregion Singleton

#define SYSTEM_VOLUME 0.2f