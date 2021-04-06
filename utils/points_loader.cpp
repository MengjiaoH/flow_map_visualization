#include "points_loader.h"

FlowMap load_points_from_raw(const std::string filename, const std::string point_type)
{
    FlowMap flow_map;
    size_t per_point_size = 0;
    if (point_type == "uint8") {
        per_point_size = 1;
    } else if (point_type == "uint16") {
        per_point_size = 2;
    } else if (point_type == "float32") {
        per_point_size = 4;
    } else if (point_type == "float64") {
        per_point_size = 8;
    } else {
        throw std::runtime_error("Unrecognized voxel type " + point_type);
    }
    std::ifstream fin(filename.c_str(), std::ios::binary);
    if (fin.is_open()){
        fin.seekg(0, std::ios::end);
        int file_size = fin.tellg();
        int num_points = file_size / (per_point_size * 3);
        // std::cout << "File size is: " << file_size << " " << num_points << std::endl;
        fin.clear();
        fin.seekg(0);
        auto point_data = std::make_shared<std::vector<uint8_t>>(num_points * per_point_size * 3, 0);
        fin.read(reinterpret_cast<char *>(point_data->data()), point_data->size());
        auto float_point_data = std::make_shared<std::vector<float>>(num_points * 3, 0.f);
        if (point_type == "uint8") {
            std::transform(point_data->begin(),
                           point_data->end(),
                           float_point_data->begin(),[](const uint8_t &x) { return float(x); });
        }else if(point_type == "uint16") {
            std::transform(reinterpret_cast<uint16_t *>(point_data->data()),
                           reinterpret_cast<uint16_t *>(point_data->data()) + num_points * 3,
                           float_point_data->begin(), [](const uint16_t &x) { return float(x); });
        }else if(point_type == "float32") {
            std::transform(reinterpret_cast<float *>(point_data->data()),
                           reinterpret_cast<float *>(point_data->data()) + num_points * 3,
                           float_point_data->begin(), [](const float &x) { return x; });
        }else{
            std::transform(reinterpret_cast<double *>(point_data->data()),
                           reinterpret_cast<double *>(point_data->data()) + num_points * 3,
                           float_point_data->begin(), [](const double &x) { return float(x); });
        }
        
        std::vector<float> &points = *float_point_data;
        for(int i = 0; i < num_points; ++i){
            flow_map.points.push_back(vec3f{points[3 * i], points[3*i+1], points[3*i+2]});
        }
        // std::cout << "float point data: " << flow_map.points.size() << "\n";
    }else{
        throw std::runtime_error("Failed to read points " + filename);
    }

    return flow_map;
}