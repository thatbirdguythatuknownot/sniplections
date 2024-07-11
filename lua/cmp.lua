local function refresh()
    io.write("\x1b[0;0H")
    io.write("\x1b[38;2;35;110;210m")
end

local OPTIONS = {
    "Boot into Terminal",
    "Check peripherals",
    "Shutdown"
}

local state = {
    option = 1
}

local START_DATE_UTC = {year = 2023, yday = 318}

local function get_year_days(yr)
    if (yr % 100 ~= 0 and {yr % 4 == 0} or {yr % 400 == 0})[1] then
        return 366
    end
    return 365
end

local function days_elapsed(now, start)
    local total_days = get_year_days(start.year) - start.yday + now.yday
    for yr = start.year + 1, now.year - 1 do
        total_days = total_days + get_year_days(yr)
    end
    return total_days
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
    if table.hour < 6 then
        table.hour = 24 - table.hour
        if table.yday == 1 then
            table.year = table.year - 1
            table.yday = get_year_days(table.year)
        else
            table.yday = table.yday - 1
        end
    else
        table.hour = table.hour - 6
    end
    local days = days_elapsed(table, START_DATE_UTC)
    local hr = h24_to_h12(table.hour)
    local time_str = os.date("Time: %%d:%M %p", os.time(table)):format(hr)
    print(hr < 10 and time_str or time_str .. ' ')
    print(("Day: %-5d"):format(days))
    print(os.date("Real date and time: %a %b %d %H:%M:%S %Y"))
end

local function print_options()
    for i = 1, #OPTIONS do
        if i == state.option then
            print("[ " .. OPTIONS[i] .. " ]")
        else
            print("  " .. OPTIONS[i] .. "  ")
        end
    end
end

local function update_screen()
    refresh()
    print("Good afternoon.\n")
    print_time_day()
    print_options()
    io.write("\x1b[0m")
end

local keyboard = require "keyboard"

keyboard.setup()       -- enable ANSI escapes
os.execute("cls")      -- clear console
io.write("\x1b[?25l")  -- hide cursor

-- clear buffer
keyboard.wasPressed(keyboard.VK_UP)
keyboard.wasPressed(keyboard.VK_DOWN)

pcall(function()
    while true do
        if keyboard.wasPressed(keyboard.VK_UP) then
            state.option = state.option - 2
        elseif not keyboard.wasPressed(keyboard.VK_DOWN) then
            goto wait_label
        end
        state.option = state.option % #OPTIONS + 1

        ::wait_label::
        keyboard.wait(0.05)
        update_screen()
    end
end)

io.write("\x1b[?25h")  -- show cursor
