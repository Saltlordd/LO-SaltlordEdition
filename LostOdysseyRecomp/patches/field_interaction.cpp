#include <stdafx.h>

extern "C" PPC_FUNC(__imp__sub_829E54F0);

PPC_FUNC(sub_829E54F0)
{
    // C1's touch-entry check calls this predicate at 82A4FBA0. The retail
    // predicate checks the committed battle/scene, but RequestBattle queues
    // its package load earlier (828278A0). Preserve every other caller.
    constexpr uint32_t TouchEntryReturn = 0x82A4FBA4;
    constexpr uint32_t BattleLoadRequest = 0x83263EA8;
    const uint32_t caller = static_cast<uint32_t>(ctx.lr);
    __imp__sub_829E54F0(ctx, base);
    if (caller == TouchEntryReturn && ctx.r3.u32 == 0 &&
        static_cast<int32_t>(PPC_LOAD_U32(BattleLoadRequest)) > 0)
    {
        // C1 takes its existing false branch before the container script
        // closes the menu or marks the container destroyed. Request states
        // 1/2 are cleared by 82826E80 on completion or cancellation.
        ctx.r3.u64 = 1;
    }
}
