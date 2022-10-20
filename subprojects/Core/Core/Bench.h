#pragma once
#include <Types.h>
#include <stdio.h>

namespace Core {

enum class BenchEnableShowOnStopAndShow : bool {
    No = false,
    Yes = true,
};

struct Bench {

    constexpr Bench(BenchEnableShowOnStopAndShow display, FILE* output_file = stderr)
        : m_out(output_file)
        , m_should_show_on_stop(display == BenchEnableShowOnStopAndShow::Yes)
    {
    }

    static u64 current_tick()
    {
        u64 tick = 0;
        u32 cycles_low = 0;
        u32 cycles_high = 0;
        asm volatile("rdtsc\n"
                     : "=d"(cycles_high), "=a"(cycles_low)::
                     "%rbx", "%rcx");
        tick = ((u64)cycles_high << 32) | cycles_low;
        return tick;
    }

    void start()
    {
        m_total_cycles += m_stop_cycle - m_start_cycle;
        m_start_cycle = current_tick();
    }

    void stop()
    {
        m_stop_cycle = current_tick();
    }

    void stop_and_show(c_string display_message)
    {
        m_stop_cycle = current_tick();
        if (m_should_show_on_stop)
            show(display_message);
    }

    void show(c_string message) const
    {
        auto cycles = m_stop_cycle - m_start_cycle;
        auto total = m_total_cycles + cycles;

        auto const* cycles_postfix = "";
        if (cycles > 1000) {
            cycles /= 1000;
            cycles_postfix = "K";
        }
        if (cycles > 1000) {
            cycles /= 1000;
            cycles_postfix = "M";
        }
        if (cycles > 1000) {
            cycles /= 1000;
            cycles_postfix = "G";
        }

        auto const* total_postfix = "";
        if (total > 1000) {
            total /= 1000;
            total_postfix = "K";
        }
        if (total > 1000) {
            total /= 1000;
            total_postfix = "M";
        }
        if (total > 1000) {
            total /= 1000;
            total_postfix = "G";
        }
        (void)fprintf(m_out, "%12s: %3lu%s cycles | total: %lu%s\n",
            message, cycles, cycles_postfix, total, total_postfix);
    }

    constexpr u64 start_cycle() const { return m_start_cycle; }
    constexpr u64 stop_cycle() const { return m_stop_cycle; }
    constexpr u64 total_cycles() const { return m_total_cycles; }

private:
    FILE* m_out { nullptr };
    u64 m_start_cycle { 0 };
    u64 m_stop_cycle { 0 };
    u64 m_total_cycles { 0 };
    bool m_should_show_on_stop { false };
};

}
