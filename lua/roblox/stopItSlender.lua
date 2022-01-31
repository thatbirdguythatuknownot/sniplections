local light = Instance.new("PointLight")
light.Enabled = true
light.Color = Color3.fromRGB(255, 255, 255)
light.Brightness = 2
light.Range = 1000
light.Shadows = false
light.Parent = game.Players.LocalPlayer.Character.HumanoidRootPart
local clickables = {}
local removed = {}
hasclickables = false
hasbboard = 0
repeat 
    for i, v in ipairs(workspace:GetDescendants())do
        if v:IsA("ClickDetector") and v.Name == 'PageCD' and not clickables[v] then
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
                tx.TextSize = 20
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
                        hasbboard = hasbboard - 1
                    end
                    bg:Remove()
                end)
                z2 = v.Parent.Off:GetPropertyChangedSignal("Transparency"):Connect(function()
                    z:Disconnect()
                    z2:Disconnect()
                    if not removed[v] then
                        removed[v] = true
                        hasbboard = hasbboard - 1
                    end
                    bg:Remove()
                end)
            else
                local tx = Instance.new("TextLabel")
                tx.Text = "Page"
                tx.Size = UDim2.new(1, 0, 1, 0)
                tx.TextColor3 = Color3.fromRGB(0, 255, 0)
                tx.Font = Enum.Font.GothamBlack
                tx.TextSize = 16
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
                    hasbboard = hasbboard - 1
                    bg:Remove()
                end)
            end
            clickables[v] = true
            hasbboard = hasbboard + 1
            hasclickables = true
            wait()
        end
    end
    wait()
until hasclickables and hasbboard == 0
