/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Scripts for spells with SPELLFAMILY_PRIEST and SPELLFAMILY_GENERIC spells used by priest players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_pri_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"

enum PriestSpells
{
    PRIEST_SPELL_GUARDIAN_SPIRIT_HEAL           = 48153,
    PRIEST_SPELL_PENANCE_R1                     = 47540,
    PRIEST_SPELL_PENANCE_R1_DAMAGE              = 47758,
    PRIEST_SPELL_PENANCE_R1_HEAL                = 47757,
    PRIEST_SPELL_REFLECTIVE_SHIELD_TRIGGERED    = 33619,
    PRIEST_SPELL_REFLECTIVE_SHIELD_R1           = 33201,
    PRIEST_SPELL_VAMPIRIC_TOUCH_DISPEL          = 64085,
    PRIEST_SPELL_EMPOWERED_RENEW                = 63544,
    PRIEST_ICON_ID_EMPOWERED_RENEW_TALENT       = 3021,
    PRIEST_ICON_ID_PAIN_AND_SUFFERING           = 2874,
    PRIEST_SHADOW_WORD_DEATH                    = 32409,
    PRIEST_SHADOWFORM_VISUAL_WITHOUT_GLYPH      = 107903,
    PRIEST_SHADOWFORM_VISUAL_WITH_GLYPH         = 107904,
    PRIEST_GLYPH_OF_SHADOW                      = 107906,

    PRIEST_SPELL_IMPROVED_POWER_WORD_SHIELD_R1  = 14748,
    PRIEST_SPELL_IMPROVED_POWER_WORD_SHIELD_R2  = 14768,
    PRIEST_SPELL_REVELATIONS                    = 88627,
    PRIEST_SPELL_RENEW                          = 139,
    PRIEST_SPELL_SANCTUARY_4YD_DUMMY            = 88667,
    PRIEST_SPELL_SANCTUARY_4YD_HEAL             = 88668,
    PRIEST_SPELL_SANCTUARY_8YD_DUMMY            = 88685,
    PRIEST_SPELL_SANCTUARY_8YD_HEAL             = 88686,
};

// Guardian Spirit
class spell_pri_guardian_spirit : public SpellScriptLoader
{
    public:
        spell_pri_guardian_spirit() : SpellScriptLoader("spell_pri_guardian_spirit") { }

        class spell_pri_guardian_spirit_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_guardian_spirit_AuraScript);

            uint32 healPct;

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(PRIEST_SPELL_GUARDIAN_SPIRIT_HEAL))
                    return false;
                return true;
            }

            bool Load()
            {
                healPct = GetSpellInfo()->Effects[EFFECT_1].CalcValue();
                return true;
            }

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                // Set absorbtion amount to unlimited
                amount = -1;
            }

            void Absorb(AuraEffect* /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
            {
                Unit* target = GetTarget();
                if (dmgInfo.GetDamage() < target->GetHealth())
                    return;

                int32 healAmount = int32(target->CountPctFromMaxHealth(healPct));
                // remove the aura now, we don't want 40% healing bonus
                Remove(AURA_REMOVE_BY_ENEMY_SPELL);
                target->CastCustomSpell(target, PRIEST_SPELL_GUARDIAN_SPIRIT_HEAL, &healAmount, NULL, NULL, true);
                absorbAmount = dmgInfo.GetDamage();
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pri_guardian_spirit_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_pri_guardian_spirit_AuraScript::Absorb, EFFECT_1);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_guardian_spirit_AuraScript();
        }
};

class spell_pri_mana_burn : public SpellScriptLoader
{
    public:
        spell_pri_mana_burn() : SpellScriptLoader("spell_pri_mana_burn") { }

        class spell_pri_mana_burn_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_mana_burn_SpellScript);

            void HandleAfterHit()
            {
                if (Unit* unitTarget = GetHitUnit())
                    unitTarget->RemoveAurasWithMechanic((1 << MECHANIC_FEAR) | (1 << MECHANIC_POLYMORPH));
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_pri_mana_burn_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_mana_burn_SpellScript;
        }
};

class spell_pri_mind_sear : public SpellScriptLoader
{
    public:
        spell_pri_mind_sear() : SpellScriptLoader("spell_pri_mind_sear") { }

        class spell_pri_mind_sear_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_mind_sear_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(Trinity::ObjectGUIDCheck(GetCaster()->GetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT)));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_mind_sear_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_mind_sear_SpellScript();
        }
};

class spell_pri_pain_and_suffering_proc : public SpellScriptLoader
{
    public:
        spell_pri_pain_and_suffering_proc() : SpellScriptLoader("spell_pri_pain_and_suffering_proc") { }

        // 47948 Pain and Suffering (proc)
        class spell_pri_pain_and_suffering_proc_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_pain_and_suffering_proc_SpellScript);

            void HandleEffectScriptEffect(SpellEffIndex /*effIndex*/)
            {
                // Refresh Shadow Word: Pain on target
                if (Unit* unitTarget = GetHitUnit())
                    if (AuraEffect* aur = unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x8000, 0, 0, GetCaster()->GetGUID()))
                        aur->GetBase()->RefreshDuration();
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pri_pain_and_suffering_proc_SpellScript::HandleEffectScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_pain_and_suffering_proc_SpellScript;
        }
};

class spell_pri_penance : public SpellScriptLoader
{
    public:
        spell_pri_penance() : SpellScriptLoader("spell_pri_penance") { }

        class spell_pri_penance_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_penance_SpellScript);

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* spellEntry)
            {
                if (!sSpellMgr->GetSpellInfo(PRIEST_SPELL_PENANCE_R1))
                    return false;
                // can't use other spell than this penance due to spell_ranks dependency
                if (sSpellMgr->GetFirstSpellInChain(PRIEST_SPELL_PENANCE_R1) != sSpellMgr->GetFirstSpellInChain(spellEntry->Id))
                    return false;

                uint8 rank = sSpellMgr->GetSpellRank(spellEntry->Id);
                if (!sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_DAMAGE, rank, true))
                    return false;
                if (!sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_HEAL, rank, true))
                    return false;

                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (Unit* unitTarget = GetHitUnit())
                {
                    if (!unitTarget->isAlive())
                        return;

                    uint8 rank = sSpellMgr->GetSpellRank(GetSpellInfo()->Id);

                    if (caster->IsFriendlyTo(unitTarget))
                        caster->CastSpell(unitTarget, sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_HEAL, rank), false, 0);
                    else
                        caster->CastSpell(unitTarget, sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_DAMAGE, rank), false, 0);
                }
            }

            SpellCastResult CheckCast()
            {
                Player* caster = GetCaster()->ToPlayer();
                if (Unit* target = GetExplTargetUnit())
                    if (!caster->IsFriendlyTo(target) && !caster->IsValidAttackTarget(target))
                        return SPELL_FAILED_BAD_TARGETS;
                return SPELL_CAST_OK;
            }

            void Register()
            {
                // add dummy effect spell handler to Penance
                OnEffectHitTarget += SpellEffectFn(spell_pri_penance_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                OnCheckCast += SpellCheckCastFn(spell_pri_penance_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_penance_SpellScript;
        }
};

// Reflective Shield
class spell_pri_reflective_shield_trigger : public SpellScriptLoader
{
    public:
        spell_pri_reflective_shield_trigger() : SpellScriptLoader("spell_pri_reflective_shield_trigger") { }

        class spell_pri_reflective_shield_trigger_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_reflective_shield_trigger_AuraScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(PRIEST_SPELL_REFLECTIVE_SHIELD_TRIGGERED) || !sSpellMgr->GetSpellInfo(PRIEST_SPELL_REFLECTIVE_SHIELD_R1))
                    return false;
                return true;
            }

            void Trigger(AuraEffect* aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
            {
                Unit* target = GetTarget();
                if (dmgInfo.GetAttacker() == target)
                    return;

                if (GetCaster())
                    if (AuraEffect* talentAurEff = target->GetAuraEffectOfRankedSpell(PRIEST_SPELL_REFLECTIVE_SHIELD_R1, EFFECT_0))
                    {
                        int32 bp = CalculatePctN(absorbAmount, talentAurEff->GetAmount());
                        target->CastCustomSpell(dmgInfo.GetAttacker(), PRIEST_SPELL_REFLECTIVE_SHIELD_TRIGGERED, &bp, NULL, NULL, true, NULL, aurEff);
                    }
            }

            void Register()
            {
                 AfterEffectAbsorb += AuraEffectAbsorbFn(spell_pri_reflective_shield_trigger_AuraScript::Trigger, EFFECT_0);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_reflective_shield_trigger_AuraScript();
        }
};

enum PrayerOfMending
{
    SPELL_T9_HEALING_2_PIECE = 67201,
};
// Prayer of Mending Heal
class spell_pri_prayer_of_mending_heal : public SpellScriptLoader
{
public:
    spell_pri_prayer_of_mending_heal() : SpellScriptLoader("spell_pri_prayer_of_mending_heal") { }

    class spell_pri_prayer_of_mending_heal_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pri_prayer_of_mending_heal_SpellScript);

        void HandleHeal(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetOriginalCaster())
            {
                if (AuraEffect* aurEff = caster->GetAuraEffect(SPELL_T9_HEALING_2_PIECE, EFFECT_0))
                {
                    int32 heal = GetHitHeal();
                    AddPctN(heal, aurEff->GetAmount());
                    SetHitHeal(heal);
                }
            }

        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_pri_prayer_of_mending_heal_SpellScript::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_pri_prayer_of_mending_heal_SpellScript();
    }
};

class spell_pri_vampiric_touch : public SpellScriptLoader
{
    public:
        spell_pri_vampiric_touch() : SpellScriptLoader("spell_pri_vampiric_touch") { }

        class spell_pri_vampiric_touch_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_vampiric_touch_AuraScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(PRIEST_SPELL_VAMPIRIC_TOUCH_DISPEL))
                    return false;
                return true;
            }

            void HandleDispel(DispelInfo* /*dispelInfo*/)
            {
                if (Unit* caster = GetCaster())
                    if (Unit* target = GetUnitOwner())
                        if (AuraEffect const* aurEff = GetEffect(EFFECT_1))
                        {
                            int32 damage = aurEff->GetAmount() * 8;
                            // backfire damage
                            caster->CastCustomSpell(target, PRIEST_SPELL_VAMPIRIC_TOUCH_DISPEL, &damage, NULL, NULL, true, NULL, aurEff);
                        }
            }

            void Register()
            {
                AfterDispel += AuraDispelFn(spell_pri_vampiric_touch_AuraScript::HandleDispel);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_vampiric_touch_AuraScript();
        }
};

class spell_priest_renew : public SpellScriptLoader
{
    public:
        spell_priest_renew() : SpellScriptLoader("spell_priest_renew") { }

        class spell_priest_renew_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_priest_renew_AuraScript);

            bool Load()
            {
                return GetCaster() && GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            void HandleApplyEffect(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    // Empowered Renew
                    if (AuraEffect const* empoweredRenewAurEff = caster->GetDummyAuraEffect(SPELLFAMILY_PRIEST, PRIEST_ICON_ID_EMPOWERED_RENEW_TALENT, EFFECT_1))
                    {
                        uint32 heal = caster->SpellHealingBonusDone(GetTarget(), GetSpellInfo(), GetEffect(EFFECT_0)->GetAmount(), DOT);
                        heal = GetTarget()->SpellHealingBonusTaken(caster, GetSpellInfo(), heal, DOT);

                        int32 basepoints0 = empoweredRenewAurEff->GetAmount() * GetEffect(EFFECT_0)->GetTotalTicks() * int32(heal) / 100;
                        caster->CastCustomSpell(GetTarget(), PRIEST_SPELL_EMPOWERED_RENEW, &basepoints0, NULL, NULL, true, NULL, aurEff);
                    }
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_priest_renew_AuraScript::HandleApplyEffect, EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_priest_renew_AuraScript();
        }
};

class spell_pri_shadow_word_death : public SpellScriptLoader
{
    public:
        spell_pri_shadow_word_death() : SpellScriptLoader("spell_pri_shadow_word_death") { }

        class spell_pri_shadow_word_death_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_shadow_word_death_SpellScript);

            void HandleDamage()
            {
                int32 damage = GetHitDamage();

                // Pain and Suffering reduces damage
                if (AuraEffect* aurEff = GetCaster()->GetDummyAuraEffect(SPELLFAMILY_PRIEST, PRIEST_ICON_ID_PAIN_AND_SUFFERING, EFFECT_1))
                    AddPctN(damage, aurEff->GetAmount());

                GetCaster()->CastCustomSpell(GetCaster(), PRIEST_SHADOW_WORD_DEATH, &damage, 0, 0, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_shadow_word_death_SpellScript::HandleDamage);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_shadow_word_death_SpellScript();
        }
};

class spell_pri_shadowform : public SpellScriptLoader
{
    public:
        spell_pri_shadowform() : SpellScriptLoader("spell_pri_shadowform") { }

        class spell_pri_shadowform_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_shadowform_AuraScript);

            bool Validate(SpellInfo const* /*entry*/)
            {
                if (!sSpellMgr->GetSpellInfo(PRIEST_SHADOWFORM_VISUAL_WITHOUT_GLYPH) ||
                    !sSpellMgr->GetSpellInfo(PRIEST_SHADOWFORM_VISUAL_WITH_GLYPH))
                    return false;
                return true;
            }

            void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                GetTarget()->CastSpell(GetTarget(), GetTarget()->HasAura(PRIEST_GLYPH_OF_SHADOW) ? PRIEST_SHADOWFORM_VISUAL_WITH_GLYPH : PRIEST_SHADOWFORM_VISUAL_WITHOUT_GLYPH, true);
            }

            void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                GetTarget()->RemoveAurasDueToSpell(GetTarget()->HasAura(PRIEST_GLYPH_OF_SHADOW) ? PRIEST_SHADOWFORM_VISUAL_WITH_GLYPH : PRIEST_SHADOWFORM_VISUAL_WITHOUT_GLYPH);
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_pri_shadowform_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_MOD_SHAPESHIFT, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                AfterEffectRemove += AuraEffectRemoveFn(spell_pri_shadowform_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_MOD_SHAPESHIFT, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_shadowform_AuraScript();
        }
};


// Power Word: Fortitude
// Spell Id: 21562
class spell_pri_power_word_fortitude : public SpellScriptLoader
{
    public:
        spell_pri_power_word_fortitude() : SpellScriptLoader("spell_pri_power_word_fortitude") { }

        class spell_pri_power_word_fortitude_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_power_word_fortitude_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    std::list<Unit*> PartyMembers;
                    caster->GetPartyMembers(PartyMembers);
                    if (PartyMembers.size() > 1)
                        caster->CastSpell(GetHitUnit(), 79105, true); // Power Word : Fortitude (Raid)
                    else
                        caster->CastSpell(GetHitUnit(), 79104, true); // Power Word : Fortitude (Caster)
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pri_power_word_fortitude_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_power_word_fortitude_SpellScript;
        }
};

// Power Word: Shield
// Spell Id: 17
class spell_pri_power_word_shield : public SpellScriptLoader
{
    public:
        spell_pri_power_word_shield() : SpellScriptLoader("spell_pri_power_word_shield") { }

        class spell_pri_power_word_shield_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_power_word_shield_AuraScript);

            void CalculateAmount(AuraEffect const* aurEff, int32 & amount, bool & /*canBeRecalculated*/)
            {
                // Improved Power Word: Shield rank 1
                if (AuraEffect const* improved = GetCaster()->GetAuraEffect(PRIEST_SPELL_IMPROVED_POWER_WORD_SHIELD_R1, EFFECT_0))
                    amount += improved->GetAmount();

                // Improved Power Word: Shield rank 2
                if (AuraEffect const* improved = GetCaster()->GetAuraEffect(PRIEST_SPELL_IMPROVED_POWER_WORD_SHIELD_R2, EFFECT_0))
                    amount += improved->GetAmount();

                amount *= GetCaster()->GetTotalAuraMultiplier(SPELL_AURA_MOD_HEALING_DONE_PERCENT);
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pri_power_word_shield_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_power_word_shield_AuraScript();
        }
};

// 81208, 81206 Chakra: Serenity and Chakra: Sanctuary spell swap supressor
class spell_pri_chakra_swap_supressor: public SpellScriptLoader
{
public:
    spell_pri_chakra_swap_supressor() : SpellScriptLoader("spell_pri_chakra_swap_supressor") {}

    class spell_pri_chakra_swap_supressor_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pri_chakra_swap_supressor_SpellScript);

        void PreventSwapApplicationOnCaster(WorldObject*& target)
        {
            // If the caster has the Revelations talent (88627) The chakra: serenity aura (81208) and the chakra: sanctuary
            // (81206) swaps the Holy Word: Chastise spell (the one that you learn when you spec into the holy tree)
            // for a Holy Word: Serenity spell (88684) or a Holy Word: Sanctuary (88684), if the caster doesnt have the
            // talent, lets just block the swap effect.
            if (!GetCaster()->HasAura(PRIEST_SPELL_REVELATIONS))
                target = NULL;
        }

        void Register()
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_pri_chakra_swap_supressor_SpellScript::PreventSwapApplicationOnCaster, EFFECT_2, TARGET_UNIT_CASTER);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_pri_chakra_swap_supressor_SpellScript();
    }
};

// 81585 Chakra: Serenity, Renew spell duration reset
class spell_pri_chakra_serenity_proc : public SpellScriptLoader
{
public:
    spell_pri_chakra_serenity_proc() : SpellScriptLoader("spell_pri_chakra_serenity_proc") {}

    class spell_pri_chakra_serenity_proc_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pri_chakra_serenity_proc_SpellScript);

        void HandleScriptEffect(SpellEffIndex /*effIndex*/)
        {
            Unit* target = GetHitUnit();

            if (!target)
                return;

            if (Aura* renew = target->GetAura(PRIEST_SPELL_RENEW, GetCaster()->GetGUID()))
                renew->RefreshDuration();
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_pri_chakra_serenity_proc_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_pri_chakra_serenity_proc_SpellScript();
    }
};

// 88685, 88687 Chakra: Sanctuary GTAoe effect
class spell_pri_chakra_sanctuary_heal: public SpellScriptLoader
{
public:
    spell_pri_chakra_sanctuary_heal() : SpellScriptLoader("spell_pri_chakra_sanctuary_heal") {}

    class spell_pri_chakra_sanctuary_heal_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pri_chakra_sanctuary_heal_SpellScript);

        float x, y, z;

        bool Load()
        {
            if (GetCaster()->GetTypeId() != TYPEID_PLAYER)
                return false;

            GetExplTargetDest()->GetPosition(x, y, z);
            return true;
        }

        void HandleExtraEffect()
        {
            if (GetSpellInfo()->Id == PRIEST_SPELL_SANCTUARY_8YD_DUMMY)
                GetCaster()->CastSpell(x, y, z, PRIEST_SPELL_SANCTUARY_4YD_DUMMY, true);
        }

        void Register()
        {
            AfterCast += SpellCastFn(spell_pri_chakra_sanctuary_heal_SpellScript::HandleExtraEffect);
        }
    };

    class spell_pri_chakra_sanctuary_heal_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_pri_chakra_sanctuary_heal_AuraScript);

        bool Load()
        {
            if (GetCaster()->GetTypeId() != TYPEID_PLAYER)
                return false;

            return true;
        }

        void HandlePeriodicDummy(AuraEffect const* aurEff)
        {
            Unit* caster = GetCaster();
            DynamicObject* dynObj = caster->GetDynObject(GetSpellInfo()->Id);

            if(caster && dynObj && caster->GetMapId() == dynObj->GetMapId())
            {
                float x, y, z;
                dynObj->GetPosition(x, y, z);

                switch(GetSpellInfo()->Id)
                {
                    case PRIEST_SPELL_SANCTUARY_8YD_DUMMY:
                    {
                        caster->CastSpell(x, y, z, PRIEST_SPELL_SANCTUARY_8YD_HEAL, true);
                        break;
                    }
                    case PRIEST_SPELL_SANCTUARY_4YD_DUMMY:
                    {
                        caster->CastSpell(x, y, z, PRIEST_SPELL_SANCTUARY_4YD_HEAL, true);
                        break;
                    }
                }
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_pri_chakra_sanctuary_heal_AuraScript::HandlePeriodicDummy, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_pri_chakra_sanctuary_heal_SpellScript();
    }
    AuraScript* GetAuraScript() const
    {
        return new spell_pri_chakra_sanctuary_heal_AuraScript();
    }
};

class DistanceCheck : public std::unary_function<Unit*, bool>
{
    public:
        explicit DistanceCheck(float _x, float _y) : x(_x), y(_y) { }
        bool operator()(WorldObject* object)
        {
            return object->GetExactDist2d(x, y) <= 3.0f;
        }

    private:
        float x, y;
};
// Used to prevent the 8yd heal from occuring if the target is in the 4yd one range
class spell_pri_chakra_sanctuary_heal_target_selector: public SpellScriptLoader
{
public:
    spell_pri_chakra_sanctuary_heal_target_selector() : SpellScriptLoader("spell_pri_chakra_sanctuary_heal_target_selector") {}

    class spell_pri_chakra_sanctuary_heal_target_selector_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pri_chakra_sanctuary_heal_target_selector_SpellScript);

        float x, y;

        bool Load()
        {
            if (GetCaster()->GetTypeId() != TYPEID_PLAYER)
                return false;

            GetExplTargetDest()->GetPosition(x, y);
            return true;
        }

        void FilterTargets(std::list<WorldObject*>& unitList)
        {
            unitList.remove_if(DistanceCheck(x, y));
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_chakra_sanctuary_heal_target_selector_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ALLY);
        }

    };

    SpellScript* GetSpellScript() const
    {
        return new spell_pri_chakra_sanctuary_heal_target_selector_SpellScript();
    }
};

// Mind Spike
// Spell Id: 73510
class spell_pri_mind_spike : public SpellScriptLoader
{
    public:
        spell_pri_mind_spike() : SpellScriptLoader("spell_pri_mind_spike") { }

        class spell_pri_mind_spike_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_mind_spike_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* target = GetHitUnit();

                if (!target)
                    return;

                target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE, GetCaster()->GetGUID());
                target->RemoveAurasByType(SPELL_AURA_PERIODIC_LEECH, GetCaster()->GetGUID());
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pri_mind_spike_SpellScript::HandleDummy, EFFECT_2, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_mind_spike_SpellScript;
        }
};

// Mind Blast
// Spell Id: 8092
class spell_pri_mind_blast : public SpellScriptLoader
{
    public:
        spell_pri_mind_blast() : SpellScriptLoader("spell_pri_mind_blast") { }

        class spell_pri_mind_blast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_mind_blast_SpellScript);

            void HandleAfterHit()
            {
                Unit* target = GetHitUnit();
                Unit* caster = GetCaster();

                if (!caster || !target)
                    return;

                // Remove Mind Spike debuff
                target->RemoveAurasDueToSpell(87178, GetCaster()->GetGUID());

                // Improved Mind blast - Mind Trauma cast
                if (AuraEffect* improvedMindBlast = caster->GetDummyAuraEffect(SPELLFAMILY_PRIEST, 95, EFFECT_1))
                    if (caster->GetShapeshiftForm() == FORM_SHADOW)
                        if (roll_chance_i(improvedMindBlast->GetAmount()))
                            caster->CastSpell(target, 48301, true);
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_pri_mind_blast_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_mind_blast_SpellScript;
        }
};

void AddSC_priest_spell_scripts()
{
    new spell_pri_guardian_spirit();
    new spell_pri_mana_burn();
    new spell_pri_pain_and_suffering_proc();
    new spell_pri_penance();
    new spell_pri_reflective_shield_trigger();
    new spell_pri_mind_sear();
    new spell_pri_prayer_of_mending_heal();
    new spell_pri_vampiric_touch();
    new spell_priest_renew();
    new spell_pri_shadow_word_death();
    new spell_pri_shadowform();
	// spelly s skyfired
    new spell_pri_power_word_fortitude();
    new spell_pri_power_word_shield();
    new spell_pri_chakra_swap_supressor();
    new spell_pri_chakra_serenity_proc();
    new spell_pri_chakra_sanctuary_heal();
    new spell_pri_chakra_sanctuary_heal_target_selector();
    new spell_pri_mind_spike();
    new spell_pri_mind_blast();
}
