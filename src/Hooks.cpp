#include "Hooks.h"
#include "Manager.h"

namespace Wind
{
    struct PostCreate
	{
		static void thunk(std::uintptr_t a_this, std::uintptr_t a_modelData, const char* a_nifPath, RE::NiPointer<RE::NiNode>& a_root, std::uint32_t& a_typeOut)
		{
			func(a_this, a_modelData, a_nifPath, a_root, a_typeOut);

			if (!a_root) {
				return;
			}

			std::string nifPath = !string::is_empty(a_nifPath) ? a_nifPath : a_root->name.c_str();
			std::string nifName = a_root->name.c_str();

			if (nifPath.empty()) {
				nifPath = nifName;
			}

		    Manager::SanitizePath(nifPath);
			Manager::SanitizePath(nifName);

			const auto manager = Manager::GetSingleton();

		    RE::BSVisit::TraverseScenegraphGeometries(a_root.get(), [&](RE::BSGeometry* a_geo) {
		        if (const auto particleSystem = netimmerse_cast<RE::NiParticleSystem*>(a_geo)) {
					const float strength = manager->GetParticleWind(nifPath, nifName, particleSystem);
					if (strength != 0.0f) {
						if (const auto newWindObject = RE::BSWindModifier::Create("ParticleWind"sv, strength)) {
							particleSystem->AddModifier(newWindObject);
						}
					}
				}
				return RE::BSVisit::BSVisitControl::kContinue;
			});
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::size_t size{ 0x1 };
	};

	void Install()
	{
		stl::write_vfunc<PostCreate>(RE::VTABLE_TESModelDB____TESProcessor[0]);
		logger::info("Installed TESModelDB::TESProcessor hook");
	}
}
