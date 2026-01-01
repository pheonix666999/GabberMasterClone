#include "KickParams.h"

juce::String KickParams::toJson() const
{
    auto obj = juce::DynamicObject::Ptr(new juce::DynamicObject());
    obj->setProperty("bodyLevel", bodyLevel);
    obj->setProperty("clickLevel", clickLevel);
    obj->setProperty("pitchStartHz", pitchStartHz);
    obj->setProperty("pitchEndHz", pitchEndHz);
    obj->setProperty("pitchTauMs", pitchTauMs);
    obj->setProperty("attackMs", attackMs);
    obj->setProperty("t12Ms", t12Ms);
    obj->setProperty("t24Ms", t24Ms);
    obj->setProperty("tailMsToMinus60Db", tailMsToMinus60Db);
    obj->setProperty("clickDecayMs", clickDecayMs);
    obj->setProperty("clickHPFHz", clickHPFHz);
    obj->setProperty("bodyOscType", bodyOscType);
    obj->setProperty("keyTracking", keyTracking);
    obj->setProperty("retriggerMode", retriggerMode);
    obj->setProperty("distortionType", distortionType);
    obj->setProperty("drive", drive);
    obj->setProperty("asymmetry", asymmetry);
    obj->setProperty("outputGainDb", outputGainDb);
    obj->setProperty("outputHPFHz", outputHPFHz);
    obj->setProperty("limiterEnabled", limiterEnabled);
    obj->setProperty("velocitySensitivity", velocitySensitivity);
    return juce::JSON::toString(juce::var(obj.get()), true);
}

KickParams KickParams::fromJson(const juce::var& obj)
{
    KickParams p;
    if (auto* o = obj.getDynamicObject())
    {
        juce::ignoreUnused(o);
        p.bodyLevel = (float)obj.getProperty("bodyLevel", p.bodyLevel);
        p.clickLevel = (float)obj.getProperty("clickLevel", p.clickLevel);
        p.pitchStartHz = (float)obj.getProperty("pitchStartHz", p.pitchStartHz);
        p.pitchEndHz = (float)obj.getProperty("pitchEndHz", p.pitchEndHz);
        p.pitchTauMs = (float)obj.getProperty("pitchTauMs", p.pitchTauMs);
        p.attackMs = (float)obj.getProperty("attackMs", p.attackMs);
        p.t12Ms = (float)obj.getProperty("t12Ms", p.t12Ms);
        p.t24Ms = (float)obj.getProperty("t24Ms", p.t24Ms);
        p.tailMsToMinus60Db = (float)obj.getProperty("tailMsToMinus60Db", p.tailMsToMinus60Db);
        p.clickDecayMs = (float)obj.getProperty("clickDecayMs", p.clickDecayMs);
        p.clickHPFHz = (float)obj.getProperty("clickHPFHz", p.clickHPFHz);
        p.bodyOscType = (int)obj.getProperty("bodyOscType", p.bodyOscType);
        p.keyTracking = (float)obj.getProperty("keyTracking", p.keyTracking);
        p.retriggerMode = (bool)obj.getProperty("retriggerMode", p.retriggerMode);
        p.distortionType = (int)obj.getProperty("distortionType", p.distortionType);
        p.drive = (float)obj.getProperty("drive", p.drive);
        p.asymmetry = (float)obj.getProperty("asymmetry", p.asymmetry);
        p.outputGainDb = (float)obj.getProperty("outputGainDb", p.outputGainDb);
        p.outputHPFHz = (float)obj.getProperty("outputHPFHz", p.outputHPFHz);
        p.limiterEnabled = (bool)obj.getProperty("limiterEnabled", p.limiterEnabled);
        p.velocitySensitivity = (float)obj.getProperty("velocitySensitivity", p.velocitySensitivity);
    }
    return p;
}

KickParams loadKickParamsJson(const juce::File& file)
{
    if (!file.existsAsFile())
        return KickParams{};
    auto text = file.loadFileAsString();
    auto parsed = juce::JSON::parse(text);
    if (parsed.isVoid() || parsed.isUndefined())
        return KickParams{};
    return KickParams::fromJson(parsed);
}

void saveKickParamsJson(const juce::File& file, const KickParams& params)
{
    file.replaceWithText(params.toJson());
}
