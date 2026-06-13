#include "CarModelUtils.h"
#include "config.h"

// Single source of truth for per-vehicle external ids (ABRP + mobile relay).
// Add a new car as ONE row here; getCarModelAbrpStr / getCarModelRelayId both read it.
namespace
{
    struct CarModelInfo
    {
        int carType;
        const char *abrpId;  // ABRP telemetry car_model
        const char *relayId; // mobile-app vehicle id (must match the evDash app)
    };

    const CarModelInfo kCarModels[] = {
    {CAR_HYUNDAI_KONA_2020_39, "hyundai:kona:19:39:other", "kona_2020_39"},
    {CAR_HYUNDAI_IONIQ_2018, "hyundai:ioniq:17:28:other", "ioniq_2018_28"},
    {CAR_HYUNDAI_IONIQ_PHEV, "hyundai:phev:17:28:other", "ioniq_2018_phev"},
    {CAR_HYUNDAI_IONIQ5_58_63, "hyundai:ioniq5:21:58:mr", "hyundai_ioniq5_58_63"},
    {CAR_HYUNDAI_IONIQ5_72, "hyundai:ioniq5:21:72:lr", "hyundai_ioniq5_72"},
    {CAR_HYUNDAI_IONIQ5_77_84, "hyundai:ioniq5:21:77:lr", "hyundai_ioniq5_77_84"},
    {CAR_HYUNDAI_IONIQ6_53, "hyundai:ioniq6:23:58:rwd", "hyundai_ioniq6_53"},
    {CAR_HYUNDAI_IONIQ6_58_63, "hyundai:ioniq6:23:58:mr", "hyundai_ioniq6_58_63"},
    {CAR_HYUNDAI_IONIQ6_77_84, "hyundai:ioniq6:23:77:lr", "hyundai_ioniq6_77_84"},
    {CAR_KIA_ENIRO_2020_64, "kia:niro:19:64:other", "eniro_2020_64"},
    {CAR_KIA_ESOUL_2020_64, "kia:soul:19:64:other", "esoul_2020_64"},
    {CAR_HYUNDAI_KONA_2020_64, "hyundai:kona:19:64:other", "kona_2020_64"},
    {CAR_KIA_ENIRO_2020_39, "kia:niro:19:39:other", "eniro_2020_39"},
    {CAR_KIA_EV6_58_63, "kia:ev6:22:58:mr", "kia_ev6_58_63"},
    {CAR_KIA_EV6_77_84, "kia:ev6:22:77:lr", "kia_ev6_77_84"},
    {CAR_KIA_EV9_100, "kia:ev9:23:100:awd", "kia_ev9_100"},
    {CAR_AUDI_Q4_35, "audi:q4:21:52:meb", "audi_q4_35"},
    {CAR_AUDI_Q4_40, "audi:q4:21:77:meb", "audi_q4_40"},
    {CAR_AUDI_Q4_45, "audi:q4:21:77:meb", "audi_q4_45"},
    {CAR_AUDI_Q4_50, "audi:q4:21:77:meb", "audi_q4_50"},
    {CAR_SKODA_ENYAQ_55, "skoda:enyaq:21:52:meb", "skoda_enyaq_55"},
    {CAR_SKODA_ENYAQ_62, "skoda:enyaq:21:55:meb", "skoda_enyaq_62"},
    {CAR_SKODA_ENYAQ_82, "skoda:enyaq:21:77:meb", "skoda_enyaq_82"},
    {CAR_VW_ID3_2021_45, "volkswagen:id3:20:45:sr", "vw_id3_2021_45"},
    {CAR_VW_ID3_2021_58, "volkswagen:id3:20:58:mr", "vw_id3_2021_58"},
    {CAR_VW_ID3_2021_77, "volkswagen:id3:20:77:lr", "vw_id3_2021_77"},
    {CAR_VW_ID4_2021_45, "volkswagen:id4:20:45:sr", "vw_id4_2021_45"},
    {CAR_VW_ID4_2021_58, "volkswagen:id4:21:52", "vw_id4_2021_58"},
    {CAR_VW_ID4_2021_77, "volkswagen:id4:21:77", "vw_id4_2021_77"},
    {CAR_RENAULT_ZOE_ZE20_22, "renault:zoe:r240:22:other", "zoe_ze20_22"},
    {CAR_RENAULT_ZOE_ZE40_41, "renault:zoe:r90:41:other", "zoe_ze40_41"},
    {CAR_RENAULT_ZOE_ZE50_52, "renault:zoe:ze50:52:other", "zoe_ze50_52"},
    {CAR_SKODA_CITIGO_E_IV, "skoda:citigoe:20:36:other", "skoda_citigo_e_iv"},
    {CAR_VW_EUP_36, "volkswagen:eup:20:36:other", "vw_eup_36"},
    {CAR_SEAT_MII_ELECTRIC_36, "seat:mii:20:36:other", "seat_mii_electric_36"},
    {CAR_PEUGEOT_E208, "peugeot:e208:20:50", "peugeot_e208"},
    {CAR_BMW_I3_2014, "bmw:i3:14:22:other", "bmwi3_2014_22"},
    };
}

String getCarModelAbrpStr(int carType)
{
    for (const auto &m : kCarModels)
        if (m.carType == carType)
            return m.abrpId;
    return "n/a";
}

String getCarModelRelayId(int carType)
{
    for (const auto &m : kCarModels)
        if (m.carType == carType)
            return m.relayId;
    return "unknown";
}
