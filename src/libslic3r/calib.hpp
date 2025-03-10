#pragma once
#define calib_pressure_advance_dd

#include "GCode.hpp"
#include "GCodeWriter.hpp"
#include "PrintConfig.hpp"

namespace Slic3r {

enum class CalibMode : int {
    Calib_None = 0,
    Calib_PA_Line,
    Calib_PA_Pattern,
    Calib_PA_Tower,
    Calib_Temp_Tower,
    Calib_Vol_speed_Tower,
    Calib_VFA_Tower,
    Calib_Retraction_tower
};

struct Calib_Params {
    Calib_Params() : mode(CalibMode::Calib_None) { };
    double start, end, step;
    bool print_numbers;
    CalibMode mode;
};

class CalibPressureAdvance {
  public:
    static float find_optimal_PA_speed(const DynamicPrintConfig &config, double line_width, double layer_height,
                                       int filament_idx = 0);

  protected:
    CalibPressureAdvance() =default;
    ~CalibPressureAdvance() =default;

    enum class DrawDigitMode {
        Left_To_Right,
        Bottom_To_Top
    };

    void delta_scale_bed_ext(BoundingBoxf& bed_ext) const { bed_ext.scale(1.0f / 1.41421f); }

    std::string move_to(Vec2d pt, GCodeWriter& writer, std::string comment = std::string());
    double      e_per_mm(
        double line_width,
        double layer_height,
        float nozzle_diameter,
        float filament_diameter,
        float print_flow_ratio
    ) const;
    double      speed_adjust(int speed) const { return speed * 60; };
    
    std::string convert_number_to_string(double num) const;
    double      number_spacing() const { return m_digit_segment_len + m_digit_gap_len; };
    std::string draw_digit(
        double startx,
        double starty,
        char c,
        CalibPressureAdvance::DrawDigitMode mode,
        double line_width,
        double e_per_mm,
        GCodeWriter& writer
    );
    std::string draw_number(
        double startx,
        double starty,
        double value,
        CalibPressureAdvance::DrawDigitMode mode,
        double line_width,
        double e_per_mm,
        double speed,
        GCodeWriter& writer
    );
    
    Vec3d m_last_pos;

    DrawDigitMode m_draw_digit_mode {DrawDigitMode::Left_To_Right};
    const double m_digit_segment_len {2};
    const double m_digit_gap_len {1};
    const std::string::size_type m_max_number_len {5};
};

class CalibPressureAdvanceLine : public CalibPressureAdvance {
public:
    CalibPressureAdvanceLine(GCode* gcodegen) :
        mp_gcodegen(gcodegen),
        m_nozzle_diameter(gcodegen->config().nozzle_diameter.get_at(0))
    { };
    ~CalibPressureAdvanceLine() { };

    std::string generate_test(double start_pa = 0, double step_pa = 0.002, int count = 50);

    void set_speed(double fast = 100.0, double slow = 20.0) {
        m_slow_speed = slow;
        m_fast_speed = fast;
    }
    
    const double& line_width() { return m_line_width; };
    bool is_delta() const;
    bool& draw_numbers() { return m_draw_numbers; }

private:
    std::string print_pa_lines(double start_x, double start_y, double start_pa, double step_pa, int num);
    
    void delta_modify_start(double& startx, double& starty, int count);

    GCode* mp_gcodegen;

    double m_nozzle_diameter;
    double m_slow_speed, m_fast_speed;
    
    const double m_height_layer {0.2};
    const double m_line_width {0.6};
    const double m_thin_line_width {0.44};
    const double m_number_line_width {0.48};
    const double m_space_y {3.5};

    double m_length_short {20.0}, m_length_long {40.0};
    bool m_draw_numbers {true};
};

struct SuggestedConfigCalibPAPattern {
    const std::vector<std::pair<std::string, double>> float_pairs {
        {"initial_layer_print_height", 0.25},
        {"layer_height", 0.2},
        {"initial_layer_speed", 30}
    };

    const std::vector<std::pair<std::string, double>> nozzle_ratio_pairs {
        {"line_width", 112.5},
        {"initial_layer_line_width", 140}
    };

    const std::vector<std::pair<std::string, int>> int_pairs {
        {"skirt_loops", 0},
        {"wall_loops", 3}
    };

    const std::pair<std::string, BrimType> brim_pair {"brim_type", BrimType::btNoBrim};
};

class CalibPressureAdvancePattern : public CalibPressureAdvance {
friend struct DrawLineOptArgs;
friend struct DrawBoxOptArgs;

public:
    CalibPressureAdvancePattern(
        const Calib_Params& params,
        const DynamicPrintConfig& config,
        bool is_bbl_machine,
        Model& model,
        const Vec3d& origin
    );

    double handle_xy_size() const { return m_handle_xy_size; };
    double handle_spacing() const { return m_handle_spacing; };
    double print_size_x() const { return object_size_x() + pattern_shift(); };
    double print_size_y() const { return object_size_y(); };
    double max_layer_z() const { return height_first_layer() + ((m_num_layers - 1) * height_layer()); };

    void generate_custom_gcodes(
        const DynamicPrintConfig& config,
        bool is_bbl_machine,
        Model& model,
        const Vec3d& origin
    );

protected:
    double speed_first_layer() const { return m_config.option<ConfigOptionFloat>("initial_layer_speed")->value; };
    double speed_perimeter() const { return m_config.option<ConfigOptionFloat>("outer_wall_speed")->value; };
    double line_width_first_layer() const { return m_config.get_abs_value("initial_layer_line_width"); };
    double line_width() const { return m_config.get_abs_value("line_width"); };
    int wall_count() const { return m_config.option<ConfigOptionInt>("wall_loops")->value; };

private:
    struct DrawLineOptArgs {
        DrawLineOptArgs(const CalibPressureAdvancePattern& p) :
            height {p.height_layer()},
            line_width {p.line_width()},
            speed {p.speed_adjust(p.speed_perimeter())}
        { };

        double height;
        double line_width;
        double speed;
        std::string comment {"Print line"};
    };

    struct DrawBoxOptArgs {
        DrawBoxOptArgs(const CalibPressureAdvancePattern& p) :
            num_perimeters {p.wall_count()},
            height {p.height_first_layer()},
            line_width {p.line_width_first_layer()},
            speed {p.speed_adjust(p.speed_first_layer())}
        { };

        bool is_filled {false};
        int num_perimeters;
        double height;
        double line_width;
        double speed;
    };

    void refresh_setup(
        const DynamicPrintConfig& config,
        bool is_bbl_machine,
        const Model& model,
        const Vec3d& origin
    );
    void _refresh_starting_point(const Model& model);
    void _refresh_writer(
        bool is_bbl_machine,
        const Model& model,
        const Vec3d& origin
    );

    double height_first_layer() const { return m_config.option<ConfigOptionFloat>("initial_layer_print_height")->value; };
    double height_layer() const { return m_config.option<ConfigOptionFloat>("layer_height")->value; };
    const int get_num_patterns() const
    {
        return std::ceil((m_params.end - m_params.start) / m_params.step + 1);
    }

    std::string draw_line(
        Vec2d to_pt,
        DrawLineOptArgs opt_args
    );
    std::string draw_box(
        double min_x,
        double min_y,
        double size_x,
        double size_y,
        DrawBoxOptArgs opt_args
    );

    double to_radians(double degrees) const { return degrees * M_PI / 180; };
    double get_distance(Vec2d from, Vec2d to) const;
    
    /* 
    from slic3r documentation: spacing = extrusion_width - layer_height * (1 - PI/4)
    "spacing" = center-to-center distance of adjacent extrusions, which partially overlap
        https://manual.slic3r.org/advanced/flow-math
        https://ellis3dp.com/Print-Tuning-Guide/articles/misconceptions.html#two-04mm-perimeters--08mm
    */
    double line_spacing() const { return line_width() - height_layer() * (1 - M_PI / 4); };
    double line_spacing_first_layer() const { return line_width_first_layer() - height_first_layer() * (1 - M_PI / 4); };
    double line_spacing_angle() const { return line_spacing() / std::sin(to_radians(m_corner_angle) / 2); };

    double object_size_x() const;
    double object_size_y() const;
    double frame_size_y() const { return std::sin(to_radians(double(m_corner_angle) / 2)) * m_wall_side_length * 2; };

    double glyph_start_x(int pattern_i = 0) const;
    double glyph_length_x() const;
    double glyph_tab_max_x() const;
    double max_numbering_height() const;

    double pattern_shift() const;

    const Calib_Params& m_params;

    DynamicPrintConfig m_config;
    GCodeWriter m_writer;
    bool m_is_delta;
    Vec3d m_starting_point;

    const double m_handle_xy_size {5};
    const double m_handle_spacing {2};
    const int m_num_layers {4};
    
    const double m_wall_side_length {30.0};
    const int m_corner_angle {90};
    const int m_pattern_spacing {2};
    const double m_encroachment {1. / 3.};

    const double m_glyph_padding_horizontal {1};
    const double m_glyph_padding_vertical {1};
};
} // namespace Slic3r
