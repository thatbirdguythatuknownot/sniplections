local gameOver = false
game.ReplicatedStorage.Remotes.GameOver.OnClientEvent:Connect(function()
    gameOver = true
end)
local light = Instance.new("PointLight")
light.Enabled = true
light.Color = Color3.fromRGB(255, 255, 255)
light.Brightness = 1
light.Range = 1000
light.Shadows = false
light.Name = "ExtendLight"
if not game.Players.LocalPlayer.Character.HumanoidRootPart:FindFirstChild("ExtendLight") then
    light.Parent = game.Players.LocalPlayer.Character.HumanoidRootPart
end
local clickables = {}
local removed = {}
hasclickables = false
repeat
spawn(function()
ply = nil
for _, plyr in pairs(game.Players:GetChildren()) do
    if plyr ~= game.Players.LocalPlayer and plyr.Team.Name:lower() == "slender" then
        ply = plyr
        break
    end
end
if ply.Character:FindFirstChild("SlenderLabel") then
    return
end
local bg = Instance.new("BillboardGui")
bg.AlwaysOnTop = true
bg.ClipsDescendants = false
bg.Enabled = true
bg.Name = "SlenderLabel"
bg.Size = UDim2.new(3, 0, 1, 0)
local tx = Instance.new("TextLabel")
tx.Text = "Slender"
tx.Size = UDim2.new(1, 0, 1, 0)
tx.TextColor3 = Color3.fromRGB(255, 0, 0)
tx.Font = Enum.Font.GothamBlack
tx.TextSize = 15
tx.BackgroundTransparency = 1
tx.Parent = bg
char = ply.Character
bg.Parent = char
while ply.Team and ply.Team.Name:lower() == "slender" do
    tx.Text = ("Slender\nDistance: %d studs"):format(math.floor(game.Players.LocalPlayer:DistanceFromCharacter(char.HumanoidRootPart.Position)))
    task.wait()
end
end)
for i, v in ipairs(workspace:GetDescendants())do if (v:IsA("ClickDetector") and v.Name == 'PageCD' and not clickables[v]) or (v:IsA("ProximityPrompt") and v.Name == "1" and not clickables[v]) then
--asfl = v.Parent.Position + v.Parent.CFrame.LookVector * 2 game.Players.LocalPlayer.Character:SetPrimaryPartCFrame(CFrame.new(asfl.X, asfl.Y, asfl.Z)) wait(3)
if v.Parent:FindFirstChildOfClass("BillboardGui") then
    continue
end
local bg = Instance.new("BillboardGui")
bg.AlwaysOnTop = true
bg.ClipsDescendants = false
bg.Enabled = true
bg.Size = UDim2.new(3, 0, 1, 0)
if v.Parent:FindFirstChild("Off") then
local tx = Instance.new("TextLabel")
tx.Text = "Generator"
tx.Size = UDim2.new(1, 0, 1, 0)
tx.TextColor3 = Color3.fromRGB(0, 255, 255)
tx.Font = Enum.Font.GothamBlack
tx.TextSize = 14
tx.BackgroundTransparency = 1
tx.Parent = bg
bg.Parent = v.Parent
if v.Parent.Off.Visible == false or v.Parent.Off.Transparency == 1 then
    clickables[v] = true
    removed[v] = true
    continue
end
spawn(function()
    while not removed[v] do
        tx.Text = ("Generator\nDistance: %d studs"):format(math.floor(game.Players.LocalPlayer:DistanceFromCharacter(v.Parent.Position)))
        task.wait()
    end
end)
local z2
local z; z = v.Parent.Off:GetPropertyChangedSignal("Visible"):Connect(function()
    z:Disconnect()
    z2:Disconnect()
    if not removed[v] then
        removed[v] = true
    end
    bg:Remove()
end)
z2 = v.Parent.Off:GetPropertyChangedSignal("Transparency"):Connect(function()
    z:Disconnect()
    z2:Disconnect()
    if not removed[v] then
        removed[v] = true
    end
    bg:Remove()
end)
elseif v.Name == "1" then
local tx = Instance.new("TextLabel")
tx.Text = "Generator"
tx.Size = UDim2.new(1, 0, 1, 0)
tx.TextColor3 = Color3.fromRGB(0, 255, 255)
tx.Font = Enum.Font.GothamBlack
tx.TextSize = 14
tx.BackgroundTransparency = 1
tx.Parent = bg
bg.Parent = v.Parent
if v.Enabled == false then
    clickables[v] = true
    removed[v.Parent.Parent] = true
    bg.Enabled = false
end
spawn(function()
    while true do
        tx.Text = ("Generator\nDistance: %d studs"):format(math.floor(game.Players.LocalPlayer:DistanceFromCharacter(v.Parent.Position)))
        task.wait()
    end
end)
z = v:GetPropertyChangedSignal("Enabled"):Connect(function()
    if v.Enabled == true then
        bg.Enabled = true
        if removed[v.Parent.Parent] then
            removed[v.Parent.Parent] = false
        end
    else
        bg.Enabled = false
        if not removed[v.Parent.Parent] then
            removed[v.Parent.Parent] = true
        end
    end
end)
elseif v:IsA("ClickDetector") and v.Name == "PageCD" then
local tx = Instance.new("TextLabel")
tx.Text = "Page"
tx.Size = UDim2.new(1, 0, 1, 0)
tx.TextColor3 = Color3.fromRGB(0, 255, 0)
tx.Font = Enum.Font.GothamBlack
tx.TextSize = 11
tx.BackgroundTransparency = 1
tx.Parent = bg
bg.Parent = v.Parent
if v.Parent.Transparency == 1 then
    clickables[v] = true
    removed[v] = true
    continue
end
spawn(function()
    while not removed[v] do
        tx.Text = ("Page\nDistance: %d studs"):format(math.floor(game.Players.LocalPlayer:DistanceFromCharacter(v.Parent.Position)))
        task.wait()
    end
end)
local z; z = v.Parent:GetPropertyChangedSignal("Transparency"):Connect(function()
    z:Disconnect()
    removed[v] = true
    bg:Remove()
end)
end
clickables[v] = true
hasclickables = true
wait()
end
end
wait()
until gameOver
