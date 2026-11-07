-- Load the production plugin functions but register just this isolated test action.
dofile(os.getenv("VK640_PLUGIN"))

function teachingProbe()
    local failures, checks = 0, 0
    local function check(ok, message)
        checks = checks + 1
        failures = failures + (ok and 0 or 1)
        print((ok and "TEACHING_PASS " or "TEACHING_FAIL ") .. message)
    end
    local ok, err = pcall(function()
        app.changeActionState("select-tool", app.C.Tool_eraser)
        vk640ToggleLine()
        check(app.getToolInfo("active").type == "pen" and app.getToolInfo("active").drawingType == "line",
              "Alt+L activates pen line from eraser")
        vk640ToggleArrow()
        check(app.getToolInfo("active").drawingType == "arrow", "Alt+V switches line to arrow")
        app.changeToolColor{tool="pen", color=0x123456}
        vk640SineOne()
        local waves = app.getStrokes("selection")
        check(#waves == 1 and #waves[1].x == 129, "one-cycle sine is one selected stroke with 129 points")
        check(app.getToolInfo("active").type == "selectRect", "sine is immediately movable with selection tool")
        check(math.abs(waves[1].y[1] - waves[1].y[129]) < 1e-7,
              "one-cycle sine begins and ends on same baseline")
        vk640CycleColor()
        waves = app.getStrokes("selection")
        check(app.getToolInfo("pen").color == 0xff0000 and waves[1].color == 0xff0000,
              "Alt+C recolors pen and actual selected ink while selection tool is active")
        app.activateAction("undo")
        waves = app.getStrokes("page")
        check(#waves == 1 and waves[1].color ~= 0xff0000, "one undo restores selected stroke color")
        app.activateAction("redo")
        waves = app.getStrokes("page")
        check(#waves == 1 and waves[1].color == 0xff0000, "redo reapplies stroke color")
        app.clearSelection()
        app.activateAction("undo") -- color
        app.activateAction("undo") -- sine creation
        check(#app.getStrokes("page") == 0, "one undo for sine insertion removes the whole wave")
        app.activateAction("redo")
        check(#app.getStrokes("page") == 1, "redo restores whole sine stroke")
        vk640SineTwo()
        waves = app.getStrokes("selection")
        check(#waves == 1 and #waves[1].x == 257, "two-cycle sine selects only its own 257-point stroke")
        local wave = waves[1]
        local visible = app.getVisiblePageRect()
        local fits = visible ~= nil
        if visible then
            for i = 1, #wave.x do
                fits = fits and wave.x[i] >= visible.x and wave.x[i] <= visible.x+visible.width
                       and wave.y[i] >= visible.y and wave.y[i] <= visible.y+visible.height
            end
        end
        check(fits, "sine fits actual visible page rectangle at current zoom")
        local span = wave.x[#wave.x] - wave.x[1]
        local amplitude = (wave.y[97] - wave.y[33])/2
        local exact = true
        for i = 1, #wave.x do
            exact = exact and math.abs(wave.y[i] - (wave.y[1] - amplitude *
                    math.sin(4*math.pi*(wave.x[i]-wave.x[1])/span))) < 1e-7
        end
        check(exact, "actual two-cycle stroke follows exact sine samples")
        app.clearSelection()
        app.activateAction("undo")
        check(#app.getStrokes("page") == 1, "undo of second wave preserves first wave")
        app.activateAction("redo")
        check(#app.getStrokes("page") == 2, "redo second wave preserves first wave")
        local before = app.getActionState("set-columns-or-rows")
        check(app.showToolbarShortcuts(true) == true, "show toolbar shortcut captions API")
        check(app.showToolbarShortcuts() == false, "toggle toolbar shortcut captions API")
        check(app.getActionState("set-columns-or-rows") == before, "captions preserve column layout")
        -- Keep the second wave selected for the screenshot.
        local all = app.getStrokes("page")
        app.addToSelection({all[#all].ref})
    end)
    if not ok then check(false, tostring(err)) end
    print(string.format("TEACHING_RESULT %s (%d checks, %d failures)", failures == 0 and "PASS" or "FAIL", checks, failures))
    io.stdout:flush()
end

function initUi()
    app.registerUi{menu="Teaching regression probe", callback="teachingProbe"}
end
