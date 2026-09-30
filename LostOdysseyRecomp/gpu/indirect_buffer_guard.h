#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gpu
{
    // Command execution is worker-owned. Track only the current nesting path,
    // so a list can still be reused by subsequent packets in the same batch.
    class IndirectBufferGuard
    {
    public:
        static constexpr uint32_t PhysicalSize = 0x20000000;
        static constexpr size_t MaxDepth = 64;
        static constexpr uint32_t MaxPackets = 1u << 24;

        enum class Result { Accepted, InvalidRange, Cycle, DepthLimit };

        static bool ValidRange(uint32_t address, uint32_t dwords)
        {
            const uint32_t offset = address & (PhysicalSize - 1);
            return !(offset & 3) && uint64_t(dwords) * 4 <= uint64_t(PhysicalSize) - offset;
        }

        void Reset(uint32_t packetBudget = MaxPackets)
        {
            m_depth = 0;
            m_packetsRemaining = packetBudget;
        }

        Result Enter(uint32_t address, uint32_t dwords)
        {
            if (!ValidRange(address, dwords)) return Result::InvalidRange;
            const Entry entry{address & (PhysicalSize - 1), dwords};
            for (size_t i = 0; i < m_depth; ++i)
                if (m_path[i].address == entry.address && m_path[i].dwords == entry.dwords)
                    return Result::Cycle;
            if (m_depth == MaxDepth) return Result::DepthLimit;
            m_path[m_depth++] = entry;
            return Result::Accepted;
        }

        void Leave() { if (m_depth) --m_depth; }
        bool ConsumePacket()
        {
            if (!m_packetsRemaining) return false;
            --m_packetsRemaining;
            return true;
        }

    private:
        struct Entry { uint32_t address = 0, dwords = 0; };
        std::array<Entry, MaxDepth> m_path{};
        size_t m_depth = 0;
        uint32_t m_packetsRemaining = MaxPackets;
    };
}
