#include "CarModelUtils.h"
#include "config.h"

String getCarModelAbrpStr(int carType)
{
    switch (carType)
    {
    case CAR_HYUNDAI_KONA_2020_39:
        return "hyundai:kona:19:39:other";
    case CAR_HYUNDAI_IONIQ_2018:
        return "hyundai:ioniq:17:28:other";
    case CAR_HYUNDAI_IONIQ_PHEV:
        return "hyundai:phev:17:28:other";
    case CAR_HYUNDAI_IONIQ5_58:
        return "hyundai:ioniq5:21:58:mr";
    case CAR_HYUNDAI_IONIQ5_72:
        return "hyundai:ioniq5:21:72:lr";
    case CAR_HYUNDAI_IONIQ5_77:
        return "hyundai:ioniq5:21:77:lr";
    case CAR_HYUNDAI_IONIQ6_77:
        return "hyundai:ioniq6:23:77:lr";
    case CAR_KIA_ENIRO_2020_64:
        return "kia:niro:19:64:other";
    case CAR_KIA_ESOUL_2020_64:
        return "kia:soul:19:64:other";
    case CAR_HYUNDAI_KONA_2020_64:
        return "hyundai:kona:19:64:other";
    case CAR_KIA_ENIRO_2020_39:
        return "kia:niro:19:39:other";
    case CAR_KIA_EV6_58:
        return "kia:ev6:22:58:mr";
    case CAR_KIA_EV6_77:
        return "kia:ev6:22:77:lr";
    case CAR_KIA_EV9_100:
        return "kia:ev9:23:100:awd";
    case CAR_AUDI_Q4_35:
        return "audi:q4:21:52:meb";
    case CAR_AUDI_Q4_40:
        return "audi:q4:21:77:meb";
    case CAR_AUDI_Q4_45:
        return "audi:q4:21:77:meb";
    case CAR_AUDI_Q4_50:
        return "audi:q4:21:77:meb";
    case CAR_SKODA_ENYAQ_55:
        return "skoda:enyaq:21:52:meb";
    case CAR_SKODA_ENYAQ_62:
        return "skoda:enyaq:21:55:meb";
    case CAR_SKODA_ENYAQ_82:
        return "skoda:enyaq:21:77:meb";
    case CAR_VW_ID3_2021_45:
        return "volkswagen:id3:20:45:sr";
    case CAR_VW_ID3_2021_58:
        return "volkswagen:id3:20:58:mr";
    case CAR_VW_ID3_2021_77:
        return "volkswagen:id3:20:77:lr";
    case CAR_VW_ID4_2021_45:
        return "volkswagen:id4:20:45:sr";
    case CAR_VW_ID4_2021_58:
        return "volkswagen:id4:21:52";
    case CAR_VW_ID4_2021_77:
        return "volkswagen:id4:21:77";
    case CAR_RENAULT_ZOE:
        return "renault:zoe:r240:22:other";
    case CAR_BMW_I3_2014:
        return "bmw:i3:14:22:other";
    default:
        return "n/a";
    }
}
