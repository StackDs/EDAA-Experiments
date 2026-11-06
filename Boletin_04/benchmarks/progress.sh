#!/usr/bin/env bash
# Adaptado de la barra uhr de los boletines 02 y 03
# https://github.com/leonardlover

uptime_cs() {
    local uptime unused
    read -r uptime unused < /proc/uptime
    printf '%s\n' "$((10#${uptime%.*} * 100 + 10#${uptime#*.}))"
}

display_progress() {
    local actual="$1" total="$2" start="$3" label="$4"
    local width=30 filled seconds bar empty
    filled=$(( width * actual / total ))
    seconds=$(( ($(uptime_cs) - start) / 100 ))
    printf -v bar '%*s' "$filled" ''
    printf -v empty '%*s' "$((width - filled))" ''
    printf '[%s%s] %d%% %d/%d | %dm %ds | %s\n' "${bar// /=}" "$empty" \
        "$((100 * actual / total))" "$actual" "$total" "$((seconds / 60))" "$((seconds % 60))" "$label"
}
