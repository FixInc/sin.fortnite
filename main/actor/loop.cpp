#include <main/actor/loop.h>
#include <main/actor.h>
#include <sdk/sdk.h>
#include <utils/math/aimbot.h>
#include <menu/menu.h>

void actor::loop()
{
    ImDrawList* drawlist = ImGui::GetBackgroundDrawList();
    double closest_distance = DBL_MAX;
    DWORD_PTR closest_mesh = 0;

    for (entities& stored_entity : cached_list)
    {
        auto local_team_id = km_ptr->read<int>(cached->local_player_state + offsets.teamid);

        auto team_id = km_ptr->read<int>(stored_entity.player_state + offsets.teamid);

        if (team_id == local_team_id)
        {
			continue;
		}
        camera_manager.get_camera();
        bone_manager.get_bone_array(stored_entity.mesh);

        auto head = bone_manager.get_bone(109);
        auto root = bone_manager.get_bone(0);

        auto head_w2s = camera_manager.w2s(head);
        auto head_box = camera_manager.w2s(Vector3(head.x, head.y, head.z + 15));
        auto root_w2s = camera_manager.w2s(root);

        auto box_height = abs(head_box.y - root_w2s.y);
        auto box_width = box_height * 0.40;

        auto distance = int(camera_manager.camera.location.Distance(head) / 100.f);

        if (config->visual.visible_check && other_manager.is_visible(stored_entity.mesh))
        {
			visible_color = ImColor(0, 247, 255, 255);
		}
        else
        {
			visible_color = ImColor(230, 92, 237, 255);
        }

        if (config->visual.skeleton)
        {
            auto bone_id_to_position = bone_manager.get_skeleton_bones(bone_pairs); //1 read ez

            for (const auto& pair : bone_pairs)
            {
                if (bone_id_to_position.find(pair.first) != bone_id_to_position.end() && bone_id_to_position.find(pair.second) != bone_id_to_position.end())
                {
                    auto start_3d = bone_id_to_position[pair.first];
                    auto end_3d = bone_id_to_position[pair.second];

                    auto start_screen = camera_manager.w2s(start_3d);
                    auto end_screen = camera_manager.w2s(end_3d);

                    drawlist->AddLine(ImVec2(start_screen.x, start_screen.y), ImVec2(end_screen.x, end_screen.y), visible_color, 2);
                }
            }
        }

        if (config->visual.box)
        {
            drawing.box_2d(drawlist, head_box.x - (box_width / 2), head_box.y, box_width, box_height, visible_color, 2, false);
        }
        else if (config->visual.filled_box)
        {
            drawing.box_2d(drawlist, head_box.x - (box_width / 2), head_box.y, box_width, box_height, visible_color, 2, true);
        }

        if (config->visual.name)
        {
            auto name = other_manager.decrypt_name(stored_entity.player_state);
            ImVec2 text_size = ImGui::CalcTextSize(name.c_str());
            drawing.outlined_text(drawlist, head_box.x - text_size.x / 2, head_box.y - 15, visible_color, name.c_str());
        }

        if (config->visual.distance)
        {
            std::string text = std::to_string(int(distance)) + ("M");
            ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
            drawing.outlined_text(drawlist, root_w2s.x - text_size.x / 2, root_w2s.y, ImColor(255, 255, 255, 255), text.c_str());
        }

        auto d_x = head_w2s.x - (monitor.width / 2);
        auto d_y = head_w2s.y - (monitor.height / 2);
        auto square_root = sqrtf(d_x * d_x + d_y * d_y);

        if (square_root < config->aim.fov && square_root < closest_distance)
        {
            closest_distance = square_root;
            closest_mesh = stored_entity.mesh;

            if (config->aim.draw.line)
            {
                drawlist->AddLine(ImVec2(monitor.width / 2, monitor.height / 2), ImVec2(head_w2s.x, head_w2s.y), ImColor(255, 255, 255, 255), 2);
            }

            if (config->aim.prediction && other_manager.projectile_speed != 0)
            {
                current_root = km_ptr->read<uintptr_t>(stored_entity.actor + offsets.root_component);
                velocity = km_ptr->read<Vector3>(current_root + 0x168);
            }
        }
    }

    if (config->misc.water_mark)
    {
        drawing.outlined_text(drawlist, 15, 15, ImColor(255, 0, 208, 255), "sin.");
        drawing.outlined_text(drawlist, 36, 15, ImColor(0, 247, 255, 255), "wtf");
    }

    if (config->misc.fps)
    {
        int offset = 31;
        if (!config->misc.water_mark)
        {
            offset = 15;
        }

        char dist[128];
        sprintf_s(dist, 128, "Fps: %.f\n", ImGui::GetIO().Framerate);
        drawing.outlined_text(drawlist, 15, 31, ImColor(255, 255, 255, 255), dist);
    }

    if (config->aim.held_weapon && cached->local_pawn)
    {
        int offset = 31;

        if (config->misc.fps)
        {
            offset = 46;
        }
        else if (config->misc.fps && !config->misc.water_mark)
        {
            offset = 31;
        }
        else if (!config->misc.fps && !config->misc.water_mark)
        {
            offset = 15;
        }

        drawing.outlined_text(drawlist, 15, offset, ImColor(255, 255, 255, 255), "Held Weapon -> ");
        drawing.outlined_text(drawlist, 112, offset, ImColor(0, 247, 255, 255), local_weapon_name.c_str());
    }

    if (config->aim.draw.fov)
    {
        drawlist->AddCircle(ImVec2(monitor.width / 2, monitor.height / 2), config->aim.fov, ImColor(0, 247, 255, 255), 200, 2.5);
    }

    if (!(config->aim.aimbot && closest_mesh && cached->local_pawn))
    {
        return;
    }

    int id;
    bone_manager.get_bone_array(closest_mesh);

    switch (config->aim.aimbone){
        case 0:
            id = 109;
            break;
        case 1:
            id = 67;
            break;
        case 2:
            id = 7;
        case 3:
            id = 2;

    }
    auto target = bone_manager.get_bone(id);

    if (config->aim.prediction && other_manager.projectile_speed != 0)
    {
        target = other_manager.predict(target, velocity, other_manager.projectile_speed, other_manager.projectile_gravity, camera_manager.camera.location.Distance(target));
    }

    auto target_w2s = camera_manager.w2s(target);

    if (config->visual.visible_check && other_manager.is_visible(closest_mesh) || !config->visual.visible_check)
    {
        if (key_handler->get(VK_RBUTTON) && !target_w2s.is_zero())
        {
            if (aim.get_cross_distance(target_w2s.x, target_w2s.y, target_w2s.z) <= config->aim.fov)
            {
                aim.perform(target_w2s.x, target_w2s.y);
            }
        }
    }
}

void actor::weapon()
{
    while (1)
    {
        local_weapon_name = other_manager.get_weapon_name(cached->local_pawn);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void actor::prediction()
{
    while (1)
    {
        if (key_handler->get(VK_INSERT) & 1)
        {
            config->misc.show_menu = !config->misc.show_menu;
        }

        auto it = weapon_properties.find(local_weapon_name);
        if (it != weapon_properties.end())
        {
            other_manager.projectile_speed = it->second.speed;
            other_manager.projectile_gravity = it->second.gravity;
        }
        else
        {
            other_manager.projectile_speed = 0;
            other_manager.projectile_gravity = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}