// vanilla_tires.h: written by tools\vanilla_tires.ps1 from the game's initial.pak. Do not edit by hand.
// The vanilla grip envelope: of the 441 vanilla truck tires (94 different grip sets), the ones no other
// tire matches or beats in ground (BodyFriction), asphalt (BodyFrictionAsphalt) and mud (SubstanceFriction) at once.
#pragma once

static const float kVanillaEnvelope[16][3] = {
    { 0.9f, 3.2f, 0.4f }, // wheels_heavy_highway_double / JAT HHD III (Highway)
    { 1.0f, 3.2f, 0.2f }, // wheels_heavy_double1_avenhorn_a15 / highway_3 (Highway)
    { 1.5f, 1.5f, 2.0f }, // wheels_azov_43_191_sprinter / tire (Allterrain)
    { 1.6f, 1.5f, 1.9f }, // wheels_kenworth_t880 / tire_1 (Allterrain)
    { 2.0f, 3.0f, 1.2f }, // wheels_scout_offroad2_jeep_wrangler / tire1 (ScoutOffroad)
    { 2.2f, 2.6f, 1.3f }, // wheels_rezvani_tank / tire (ScoutOffroad)
    { 2.8f, 1.2f, 1.8f }, // wheels_mack_defense_m917 / offroad (Offroad)
    { 2.85f, 1.2f, 1.78f }, // wheels_mercedes_mamute_1519 / tire_offroad_mercedes_mamute (Offroad)
    { 3.0f, 0.5f, 8.0f }, // wheels_scout_btr / offroad_btr_1 (Mudtires)
    { 3.0f, 0.8f, 3.5f }, // wheels_heavy_offroad_p512 / offroad (Offroad)
    { 3.0f, 1.2f, 1.3f }, // wheels_heavy_single_s / chains_1 (Chains)
    { 3.1f, 0.8f, 3.4f }, // wheels_kenworth_990 / offroad (Offroad)
    { 3.1f, 1.0f, 1.9f }, // wheels_plad_440 / offroad_plad_440 (Offroad)
    { 3.2f, 0.8f, 1.5f }, // wheels_heavy_double2_avenhorn_a15 / offroad_2 (Offroad)
    { 3.2f, 1.0f, 1.3f }, // wheels_heavy_offroad_single / JAT OHS II (Offroad)
    { 3.3f, 0.6f, 2.2f }, // wheels_scout_burlak_offroad / tire (Offroad)
};
