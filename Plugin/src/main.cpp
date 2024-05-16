class Config
{
public:
	class General
	{
	public:
		inline static DKUtil::Alias::Boolean bNoFastTravelInInteriors{ "bNoFastTravelInInteriors", "General" };
		inline static DKUtil::Alias::Boolean bNoFastTravelOutsideShip{ "bNoFastTravelOutsideShip", "General" };
	};

	static void Load()
	{
		static auto MainConfig = COMPILE_PROXY("BakaFastTravelRestrictions.ini");
		MainConfig.Bind(General::bNoFastTravelInInteriors, false);
		MainConfig.Bind(General::bNoFastTravelOutsideShip, false);
		MainConfig.Load();
	}
};

class Hooks
{
public:
	static void Install()
	{
		hkCanFastTravel<153381, 0xC3>::Install();
		hkCanFastTravel<153382, 0xB1>::Install();
	}

private:
	template <std::uintptr_t ID, std::ptrdiff_t OFF>
	class hkCanFastTravel
	{
	public:
		static void Install()
		{
			static REL::Relocation<std::uintptr_t> target{ REL::ID(ID), OFF };
			auto& trampoline = SFSE::GetTrampoline();
			_OtherEventEnabled = trampoline.write_call<5>(target.address(), OtherEventEnabled);
		}

	private:
		static bool OtherEventEnabled(void* a_this, std::uint32_t a_otherEventFlags)
		{
			auto result = _OtherEventEnabled(a_this, a_otherEventFlags);
			if (result)
			{
				if (auto PlayerCharacter = RE::PlayerCharacter::GetSingleton())
				{
					if (*Config::General::bNoFastTravelInInteriors)
					{
						if (!PlayerCharacter->GetSpaceship())
						{
							if (auto parentCell = RE::stl::adjust_pointer<void*>(PlayerCharacter, 0xA8))
							{
								if (auto cellFlags = RE::stl::adjust_pointer<std::uint32_t>(*parentCell, 0x40))
								{
									if ((*cellFlags) & 1)
									{
										return false;
									}
								}
							}
						}
					}

					if (*Config::General::bNoFastTravelOutsideShip)
					{
						if (!PlayerCharacter->GetSpaceship())
						{
							return false;
						}
					}
				}
			}

			return result;
		}

		inline static REL::Relocation<decltype(&OtherEventEnabled)> _OtherEventEnabled;
	};
};

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type)
		{
		case SFSE::MessagingInterface::kPostLoad:
		{
			Hooks::Install();
			break;
		}
		default:
			break;
		}
	}
}

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
#ifndef NDEBUG
	MessageBoxA(NULL, "Loaded. You can now attach the debugger or continue execution.", Plugin::NAME.data(), NULL);
#endif

	SFSE::Init(a_sfse, false);

	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));
	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	SFSE::AllocTrampoline(1 << 6);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	Config::Load();

	return true;
}
