local keyboard = require "keyboard"

local COLORS_FORE = {
    RESET = "\x1b[0m",
    BLUE = "\x1b[38;2;35;110;210m"
}

local state = {
    option = 1,
    display_option = 0,
}

local update_screen

local function clear_screen(hard_reset)
    if hard_reset then
        keyboard.flush_pressed()
        os.execute("cls")
    else
        io.write(("\x1b[2J\x1b[0;0H"):format(code))
    end
end

local function refresh(do_hard_reset)
    if do_hard_reset ~= nil then
        clear_screen(do_hard_reset)
    else
        io.write("\x1b[0;0H")
    end
end

local function clear_key_buffers()
    keyboard.wasPressed(keyboard.VK_UP)
    keyboard.wasPressed(keyboard.VK_DOWN)
    keyboard.wasPressed(keyboard.VK_RETURN)
end

local TERMINAL_COMMANDS = {
    ["menu"] = function()
        return 0, true
    end
}

local OPTIONS = {
    [1] = {text = "Boot into Terminal", handler = function()
        print(
"Welcome to the terminal.\n\
You can go back to the start menu with the \"menu\" command."
        )
        io.write("\x1b[?25h")  -- show cursor
        while true do
            io.write(">")
            local command = io.read():gsub("^%s*(.-)%s*$", "%1")
            local handler = TERMINAL_COMMANDS[command]
            if handler then
                local success, res_0, break_loop = pcall(handler)
                if success then
                    if res_0 ~= 0 then
                        print(
                            ("command returned non-zero exit code %d")
                            :format(res_0)
                        )
                    end
                    if break_loop then
                        break
                    end
                else
                    print("command error!")
                    print(res_0)
                end
            else
                print(("no such command: '%s'"):format(command))
            end
        end
    end},
    [2] = {text = "Check peripherals", handler = function()
        print("Unimplemented.")
        keyboard.wait(1)
    end},
    [3] = {text = "Shutdown", handler = function()
        io.write(COLORS_FORE.BLUE)
        print("Goodnight.")
        keyboard.wait(1)
        error(nil)
    end}
}

local START_DATE_UTC = {year = 2024, yday = 189, hour = 6, min = 48, sec = 49}

local function get_year_days(yr)
    if (yr % 100 ~= 0 and {yr % 4 == 0} or {yr % 400 == 0})[1] then
        return 366
    end
    return 365
end

local function days_elapsed(now, start)
    local total_days
    if start.year == now.year then
        total_days = now.yday - start.yday
    else
        local total_days = get_year_days(start.year) - start.yday + now.yday
        for yr = start.year + 1, now.year - 1 do
            total_days = total_days + get_year_days(yr)
        end
    end
    if total_days > 0 then
        if now.hour < start.hour or now.hour == start.hour and (
            now.min < start.min or now.min == start.min and
                now.sec < start.sec
        ) then
            return total_days - 1, false
        end
    end
    return total_days, true
end

local MIN2SEC = 60
local HR2MIN = 60
local DAY2HR = 24
local HR2SEC = MIN2SEC * HR2MIN
local DAY2SEC = HR2SEC * DAY2HR

local MCDAY_2_IRLMIN = 20
local MCDAY_2_IRLSEC = MCDAY_2_IRLMIN * MIN2SEC
local IRLHR_2_MCDAY = HR2MIN // MCDAY_2_IRLMIN
local IRLDAY_2_MCDAY = DAY2HR * IRLHR_2_MCDAY
local IRLSEC_2_MCHR = MCDAY_2_IRLSEC // DAY2HR

local function mc_time_elapsed(now, start)
    local days, exceeded_time = days_elapsed(now, start)
    local mc_days = days * IRLDAY_2_MCDAY
    local now_secs = now.hour * HR2SEC + now.min * MIN2SEC + now.sec
    local start_secs = start.hour * HR2SEC + start.min * MIN2SEC + start.sec
    local elapsed_secs = (now_secs - start_secs) % DAY2SEC
    mc_days = mc_days + elapsed_secs // MCDAY_2_IRLSEC
    local rem = elapsed_secs % MCDAY_2_IRLSEC
    local mc_hours = rem // IRLSEC_2_MCHR
    -- transform range: 0 .. IRLSEC_2_MCHR -> 0 .. 60
    local mc_mins = rem % IRLSEC_2_MCHR * 60 // IRLSEC_2_MCHR
    return {days = mc_days, hour = mc_hours, min = mc_mins}
end

local function h24_to_h12(hr)
    hr = hr % 12
    if hr == 0 then
        return 12
    end
    return hr
end

local function print_time_day()
    local table = os.date("!*t")
    local mc_time = mc_time_elapsed(table, START_DATE_UTC)
    if mc_time.hour > 5 and mc_time.hour <= 12 then
        print("Good morning.  \n")
    elseif mc_time.hour > 12 and mc_time.hour <= 18 then
        print("Good afternoon.\n")
    else
        print("Good evening.  \n")
    end
    local hr = h24_to_h12(mc_time.hour)
    local am_or_pm = "AM"
    if mc_time.hour >= 12 then
        am_or_pm = "PM"
    end
    local time_str = ("Time: %d:%02d %s"):format(hr, mc_time.min, am_or_pm)
    print(hr < 10 and time_str .. ' ' or time_str)
    print(("Day: %-5d"):format(mc_time.days))
    print(os.date("Real date and time: %a %b %d %H:%M:%S %Y"))
end

local function print_options()
    for i = 1, #OPTIONS do
        if i == state.option then
            print("[ " .. OPTIONS[i].text .. " ]")
        else
            print("  " .. OPTIONS[i].text .. "  ")
        end
    end
end

update_screen = function(reloading_start_menu)
    if state.display_option == 0 then
        local refresh_option = nil
        if not reloading_start_menu then
            refresh_option = false
        end
        refresh(refresh_option)

        io.write("\x1b[?25l")  -- hide cursor
        io.write(COLORS_FORE.BLUE)
        print_time_day()
        print_options()
    else
        local setup = OPTIONS[state.display_option].setup
        if setup then
            setup()
        end

        refresh(true)

        coroutine.wrap(OPTIONS[state.display_option].handler)()
        clear_key_buffers()
        state.display_option = 0
        update_screen()
    end
end

keyboard.setup()  -- enable ANSI escapes
clear_screen(true)
clear_key_buffers()

local success, res = pcall(function()
    while true do
        if keyboard.wasPressed(keyboard.VK_RETURN) then
            state.display_option = state.option
            update_screen()
            goto loop_end
        elseif keyboard.wasPressed(keyboard.VK_UP) then
            state.option = state.option - 2
        elseif not keyboard.wasPressed(keyboard.VK_DOWN) then
            goto wait_label
        end
        state.option = state.option % #OPTIONS + 1

        ::wait_label::
        keyboard.wait(0.05)
        update_screen(true)

        ::loop_end::
    end
end)

io.write(COLORS_FORE.RESET)  -- clear color
io.write("\x1b[?25h")  -- show cursor

clear_screen(true)  -- hard reset

if not success and res then
    print(res)
end
