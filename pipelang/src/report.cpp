#include "pipelang/report.hpp"

#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>

namespace pipelang {
namespace {

std::string escape(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char character : text) {
        switch (character) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += character; break;
        }
    }
    return escaped;
}

std::string fixed(double value, int digits) {
    std::ostringstream text;
    text << std::fixed << std::setprecision(digits) << value;
    return text.str();
}

// Shortens to at most `limit` bytes plus an ellipsis, never splitting a
// UTF-8 character.
std::string truncate(const std::string& text, std::size_t limit) {
    if (text.size() <= limit) {
        return text;
    }
    std::size_t cut = limit - 1;
    while (cut > 0 && (static_cast<unsigned char>(text[cut]) & 0xC0) == 0x80) {
        --cut;
    }
    return text.substr(0, cut) + "\xE2\x80\xA6";
}

const char* opcode_class(IROpcode opcode) {
    switch (opcode) {
        case IROpcode::Input:
        case IROpcode::Output: return "op-io";
        case IROpcode::Clean: return "op-clean";
        case IROpcode::Filter: return "op-filter";
        case IROpcode::Transform: return "op-transform";
        case IROpcode::Sort:
        case IROpcode::Group: return "op-order";
        case IROpcode::Aggregate: return "op-aggregate";
    }
    return "op-io";
}

std::vector<std::string> dataset_order(const IRProgram& program) {
    std::vector<std::string> datasets;
    for (const IRInstruction& instruction : program) {
        if (std::find(datasets.begin(), datasets.end(), instruction.dataset) == datasets.end()) {
            datasets.push_back(instruction.dataset);
        }
    }
    return datasets;
}

// Colour index of each dataset, shared by the DAG lanes and the timeline.
std::map<std::string, std::size_t> dataset_colours(const IRProgram& program) {
    std::map<std::string, std::size_t> colours;
    for (const std::string& dataset : dataset_order(program)) {
        colours.emplace(dataset, colours.size() % 6);
    }
    return colours;
}

// Lanes are datasets (left to right in order of appearance) and rows are
// execution levels, so nodes in the same row could run at the same time.
void write_dag_svg(
    const IRProgram& program,
    const PipelineDag& dag,
    const std::map<std::string, std::size_t>& colours,
    std::ostream& output
) {
    constexpr int node_width = 196;
    constexpr int node_height = 54;
    constexpr int gap_x = 28;
    constexpr int gap_y = 30;
    constexpr int header = 34;
    constexpr int margin = 12;

    const std::vector<std::string> datasets = dataset_order(program);
    std::map<std::string, std::size_t> lane_of;
    for (std::size_t lane = 0; lane < datasets.size(); ++lane) lane_of[datasets[lane]] = lane;

    std::vector<std::size_t> level_of(program.size(), 0);
    const auto& levels = dag.execution_levels();
    for (std::size_t level = 0; level < levels.size(); ++level) {
        for (const std::size_t node : levels[level]) level_of[node] = level;
    }

    const int lanes = static_cast<int>(std::max<std::size_t>(datasets.size(), 1));
    const int rows = static_cast<int>(std::max<std::size_t>(levels.size(), 1));
    const int width = margin * 2 + lanes * node_width + (lanes - 1) * gap_x;
    const int height = margin * 2 + header + rows * node_height + (rows - 1) * gap_y;

    auto node_x = [&](std::size_t node) {
        return margin + static_cast<int>(lane_of[program[node].dataset]) * (node_width + gap_x);
    };
    auto node_y = [&](std::size_t node) {
        return margin + header + static_cast<int>(level_of[node]) * (node_height + gap_y);
    };

    output << "<svg class=\"dag\" viewBox=\"0 0 " << width << ' ' << height
           << "\" width=\"" << width << "\" role=\"img\" aria-label=\"Pipeline dependency graph\">\n"
           << "<defs><marker id=\"arrow\" viewBox=\"0 0 10 10\" refX=\"9\" refY=\"5\" "
              "markerWidth=\"7\" markerHeight=\"7\" orient=\"auto-start-reverse\">"
              "<path d=\"M0 0L10 5L0 10z\" class=\"arrow-head\"/></marker></defs>\n";

    for (std::size_t lane = 0; lane < datasets.size(); ++lane) {
        const int x = margin + static_cast<int>(lane) * (node_width + gap_x);
        output << "<rect class=\"lane lane-" << colours.at(datasets[lane]) << "\" x=\"" << x - 6
               << "\" y=\"" << margin - 4 << "\" width=\"" << node_width + 12
               << "\" height=\"" << height - 2 * margin + 8 << "\" rx=\"12\"/>\n"
               << "<text class=\"lane-label\" x=\"" << x + node_width / 2 << "\" y=\""
               << margin + 18 << "\" text-anchor=\"middle\">" << escape(datasets[lane])
               << "</text>\n";
    }

    for (const DependencyEdge& edge : dag.edges()) {
        const int x1 = node_x(edge.from) + node_width / 2;
        const int y1 = node_y(edge.from) + node_height;
        const int x2 = node_x(edge.to) + node_width / 2;
        const int y2 = node_y(edge.to);
        output << "<line class=\"edge\" x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2
               << "\" y2=\"" << y2 - 2 << "\" marker-end=\"url(#arrow)\"/>\n";
    }

    for (const IRInstruction& instruction : program) {
        const int x = node_x(instruction.id);
        const int y = node_y(instruction.id);
        const std::string detail = instruction_detail(instruction);
        output << "<g class=\"node " << opcode_class(instruction.opcode) << "\">"
               << "<title>[" << instruction.id << "] " << ir_opcode_name(instruction.opcode)
               << ' ' << escape(instruction.dataset);
        if (!detail.empty()) output << ": " << escape(detail);
        output << " (line " << instruction.line << ")</title>"
               << "<rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << node_width
               << "\" height=\"" << node_height << "\" rx=\"9\"/>"
               << "<text class=\"node-title\" x=\"" << x + 12 << "\" y=\"" << y + 22 << "\">"
               << "<tspan class=\"node-id\">" << instruction.id << "</tspan> "
               << ir_opcode_name(instruction.opcode) << "</text>";
        if (!detail.empty()) {
            output << "<text class=\"node-detail\" x=\"" << x + 12 << "\" y=\"" << y + 41
                   << "\">" << escape(truncate(detail, 30)) << "</text>";
        }
        output << "</g>\n";
    }
    output << "</svg>\n";
}

// One lane per worker thread; each bar is one DAG node, coloured by dataset.
void write_timeline_svg(
    const IRProgram& program,
    const ExecutionResult& execution,
    const std::map<std::string, std::size_t>& colours,
    std::ostream& output
) {
    constexpr int label_width = 86;
    constexpr int chart_width = 820;
    constexpr int lane_height = 34;
    constexpr int axis_height = 30;
    constexpr int margin = 10;

    double total = 0;
    for (const NodeRun& run : execution.runs) total = std::max(total, run.end_ms);
    if (total <= 0) total = 1e-3;

    const int lanes = static_cast<int>(std::max(1u, execution.workers));
    const int width = label_width + chart_width + margin * 2;
    const int height = margin * 2 + lanes * lane_height + axis_height;
    auto x_of = [&](double ms) {
        return label_width + margin + ms / total * chart_width;
    };

    output << "<svg class=\"timeline\" viewBox=\"0 0 " << width << ' ' << height
           << "\" role=\"img\" aria-label=\"Execution timeline\">\n";

    for (int lane = 0; lane < lanes; ++lane) {
        const int y = margin + lane * lane_height;
        output << "<rect class=\"track\" x=\"" << label_width + margin << "\" y=\"" << y + 4
               << "\" width=\"" << chart_width << "\" height=\"" << lane_height - 8
               << "\" rx=\"6\"/>"
               << "<text class=\"axis-label\" x=\"" << margin << "\" y=\"" << y + lane_height / 2 + 4
               << "\">Worker " << lane << "</text>\n";
    }

    for (int tick = 0; tick <= 4; ++tick) {
        const double ms = total * tick / 4;
        const double x = x_of(ms);
        const int y = margin + lanes * lane_height;
        output << "<line class=\"tick\" x1=\"" << fixed(x, 1) << "\" y1=\"" << margin
               << "\" x2=\"" << fixed(x, 1) << "\" y2=\"" << y + 4 << "\"/>"
               << "<text class=\"axis-label\" x=\"" << fixed(x, 1) << "\" y=\"" << y + 20
               << "\" text-anchor=\"" << (tick == 0 ? "start" : (tick == 4 ? "end" : "middle"))
               << "\">" << fixed(ms, ms < 10 ? 3 : 1) << " ms</text>\n";
    }

    for (const NodeRun& run : execution.runs) {
        const IRInstruction& instruction = program[run.instruction];
        const double x = x_of(run.start_ms);
        const double bar = std::max(2.0, x_of(run.end_ms) - x);
        const int y = margin + static_cast<int>(run.worker) * lane_height + 6;
        output << "<g class=\"bar bar-" << colours.at(instruction.dataset) << "\"><title>["
               << run.instruction << "] " << ir_opcode_name(instruction.opcode) << ' '
               << escape(instruction.dataset) << ": " << fixed(run.end_ms - run.start_ms, 3)
               << " ms, rows " << run.rows_in << " \xE2\x86\x92 " << run.rows_out
               << ", worker " << run.worker << "</title>"
               << "<rect x=\"" << fixed(x, 2) << "\" y=\"" << y << "\" width=\""
               << fixed(bar, 2) << "\" height=\"" << lane_height - 12 << "\" rx=\"4\"/>";
        if (bar > 54) {
            output << "<text x=\"" << fixed(x + 6, 2) << "\" y=\"" << y + 15 << "\">"
                   << run.instruction << ' ' << ir_opcode_name(instruction.opcode) << "</text>";
        }
        output << "</g>\n";
    }
    output << "</svg>\n";
}

void write_ir_listing(const IRProgram& program, std::ostream& output) {
    std::ostringstream text;
    print_ir(program, text);
    output << "<pre class=\"code\">" << escape(text.str()) << "</pre>\n";
}

void write_table(const Table& table, std::size_t limit, std::ostream& output) {
    output << "<div class=\"table-wrap\"><table><thead><tr>";
    for (const Column& column : table.columns) {
        output << "<th>" << escape(column.name) << "<span class=\"type\">"
               << column_type_name(column.type) << "</span></th>";
    }
    output << "</tr></thead><tbody>\n";
    const std::size_t shown = std::min(limit, table.rows.size());
    for (std::size_t index = 0; index < shown; ++index) {
        output << "<tr>";
        for (std::size_t column = 0; column < table.columns.size(); ++column) {
            const Value& value = table.rows[index][column];
            const bool number = value.is_number();
            output << "<td" << (number ? " class=\"num\"" : "") << ">";
            if (value.is_null()) {
                output << "<span class=\"null\">null</span>";
            } else {
                output << escape(format_value(value));
            }
            output << "</td>";
        }
        output << "</tr>\n";
    }
    output << "</tbody></table></div>\n";
    if (shown < table.rows.size()) {
        output << "<p class=\"muted\">Showing the first " << shown << " of "
               << table.rows.size() << " rows.</p>\n";
    }
}

void write_stat(const std::string& label, const std::string& value, std::ostream& output) {
    output << "<div class=\"stat\"><div class=\"stat-value\">" << escape(value)
           << "</div><div class=\"stat-label\">" << escape(label) << "</div></div>\n";
}

const char* kStyle = R"CSS(
:root {
  --bg: #f6f7fb; --panel: #ffffff; --text: #18202f; --muted: #5d6879; --border: #dfe3ea;
  --code-bg: #f3f5f9; --accent: #3355d6; --edge: #7b8494; --track: #eef1f6;
  --io: #dbe7ff; --io-b: #6f8fe0; --clean: #dcf3e3; --clean-b: #4fa36b;
  --filter: #fdf0cc; --filter-b: #c99a1e; --transform: #ebe4fd; --transform-b: #8a6ad6;
  --order: #fbe1ee; --order-b: #c45a8d; --aggregate: #ffe7d3; --aggregate-b: #d7803a;
  --d0: #3f68d9; --d1: #2f9a5b; --d2: #c98a12; --d3: #8a57d6; --d4: #cf4b4b; --d5: #1c93a8;
  --lane-alpha: 0.07;
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #11151c; --panel: #181e27; --text: #e5e9f0; --muted: #9aa5b5; --border: #2a3240;
    --code-bg: #121820; --accent: #8aa4ff; --edge: #8b95a5; --track: #202835;
    --io: #22314f; --io-b: #6f8fe0; --clean: #1d3a2a; --clean-b: #5fbf80;
    --filter: #3b3217; --filter-b: #d9ad3a; --transform: #2e2650; --transform-b: #a68af0;
    --order: #3f2333; --order-b: #e07aab; --aggregate: #42301f; --aggregate-b: #ec9a52;
    --d0: #6f93ff; --d1: #4fc282; --d2: #e2a83a; --d3: #a985f0; --d4: #ef7272; --d5: #3dbbd1;
    --lane-alpha: 0.10;
  }
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--text);
  font: 15px/1.55 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif; }
main { max-width: 1180px; margin: 0 auto; padding: 32px 20px 48px; }
header h1 { margin: 0 0 4px; font-size: 28px; letter-spacing: -0.01em; }
header p { margin: 0; color: var(--muted); }
code, pre, .mono, .node-detail { font-family: ui-monospace, "Cascadia Code", Consolas, monospace;
  font-variant-ligatures: none; }
.stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(128px, 1fr)); gap: 12px; margin: 24px 0; }
.stat { background: var(--panel); border: 1px solid var(--border); border-radius: 12px; padding: 14px 16px; }
.stat-value { font-size: 21px; font-weight: 650; font-variant-numeric: tabular-nums; white-space: nowrap; }
.stat-label { color: var(--muted); font-size: 13px; }
section { background: var(--panel); border: 1px solid var(--border); border-radius: 14px; padding: 20px 22px; margin: 18px 0; }
section h2 { margin: 0 0 4px; font-size: 19px; }
section > p.lead { margin: 0 0 16px; color: var(--muted); }
h3 { font-size: 15px; margin: 18px 0 8px; }
.muted { color: var(--muted); font-size: 13px; }
.grid-2 { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 18px; }
.scroll { overflow-x: auto; }
.stacked > div + div { margin-top: 8px; }
pre.code { background: var(--code-bg); border: 1px solid var(--border); border-radius: 10px;
  padding: 12px 14px; margin: 0; overflow-x: auto; font-size: 13px; line-height: 1.6; }
.source { counter-reset: line; }
.source span { display: block; counter-increment: line; white-space: pre; }
.source span::before { content: counter(line); display: inline-block; width: 2.4em; margin-right: 1em;
  text-align: right; color: var(--muted); opacity: 0.7; }
.warning { border-left: 4px solid var(--filter-b); background: var(--code-bg); padding: 8px 12px;
  border-radius: 6px; margin: 6px 0; }
ol.steps { margin: 0; padding-left: 0; list-style: none; }
ol.steps li { padding: 8px 0; border-bottom: 1px solid var(--border); }
ol.steps li:last-child { border-bottom: 0; }
.pass { display: inline-block; font-size: 12px; font-weight: 600; padding: 1px 8px; border-radius: 999px;
  background: var(--io); color: var(--text); margin-right: 8px; }
.line-ref { color: var(--muted); font-size: 13px; margin-right: 6px; }
svg text { fill: var(--text); font-size: 12px; font-family: system-ui, -apple-system, "Segoe UI", sans-serif; }
svg.dag { max-width: 100%; height: auto; display: block; }
svg.timeline { width: 100%; height: auto; display: block; }
.lane { stroke: none; }
.lane-0 { fill: var(--d0); fill-opacity: var(--lane-alpha); } .lane-1 { fill: var(--d1); fill-opacity: var(--lane-alpha); }
.lane-2 { fill: var(--d2); fill-opacity: var(--lane-alpha); } .lane-3 { fill: var(--d3); fill-opacity: var(--lane-alpha); }
.lane-4 { fill: var(--d4); fill-opacity: var(--lane-alpha); } .lane-5 { fill: var(--d5); fill-opacity: var(--lane-alpha); }
.lane-label { font-weight: 650; font-size: 13px; }
.edge { stroke: var(--edge); stroke-width: 1.6; }
.arrow-head { fill: var(--edge); }
.node rect { stroke-width: 1.4; }
.node-title { font-weight: 650; font-size: 12.5px; }
.node-id { fill: var(--muted); font-weight: 500; }
.node-detail { font-family: ui-monospace, Consolas, monospace; font-size: 11.5px; fill: var(--muted); }
.op-io rect { fill: var(--io); stroke: var(--io-b); }
.op-clean rect { fill: var(--clean); stroke: var(--clean-b); }
.op-filter rect { fill: var(--filter); stroke: var(--filter-b); }
.op-transform rect { fill: var(--transform); stroke: var(--transform-b); }
.op-order rect { fill: var(--order); stroke: var(--order-b); }
.op-aggregate rect { fill: var(--aggregate); stroke: var(--aggregate-b); }
.legend { display: flex; flex-wrap: wrap; gap: 8px 16px; margin: 12px 0 0; font-size: 13px; color: var(--muted); }
.legend span::before { content: ""; display: inline-block; width: 12px; height: 12px; border-radius: 3px;
  margin-right: 6px; vertical-align: -1px; border: 1px solid; }
.lg-io::before { background: var(--io); border-color: var(--io-b) !important; }
.lg-clean::before { background: var(--clean); border-color: var(--clean-b) !important; }
.lg-filter::before { background: var(--filter); border-color: var(--filter-b) !important; }
.lg-transform::before { background: var(--transform); border-color: var(--transform-b) !important; }
.lg-order::before { background: var(--order); border-color: var(--order-b) !important; }
.lg-aggregate::before { background: var(--aggregate); border-color: var(--aggregate-b) !important; }
.track { fill: var(--track); }
.tick { stroke: var(--border); stroke-dasharray: 3 4; }
.axis-label { fill: var(--muted); font-size: 11.5px; }
.bar text { fill: #ffffff; font-size: 11px; font-weight: 600; }
.bar-0 rect { fill: var(--d0); } .bar-1 rect { fill: var(--d1); } .bar-2 rect { fill: var(--d2); }
.bar-3 rect { fill: var(--d3); } .bar-4 rect { fill: var(--d4); } .bar-5 rect { fill: var(--d5); }
.table-wrap { overflow-x: auto; border: 1px solid var(--border); border-radius: 10px; }
table { border-collapse: collapse; width: 100%; font-size: 13.5px; }
th, td { padding: 7px 12px; text-align: left; border-bottom: 1px solid var(--border); white-space: nowrap; }
th { background: var(--code-bg); font-weight: 600; position: sticky; top: 0; }
th .type { display: block; font-weight: 400; font-size: 11px; color: var(--muted); }
tbody tr:last-child td { border-bottom: 0; }
td.num { text-align: right; font-variant-numeric: tabular-nums; }
.null { color: var(--muted); font-style: italic; }
footer { color: var(--muted); font-size: 13px; text-align: center; margin-top: 28px; }
@media (max-width: 640px) { main { padding: 20px 16px 36px; } section { padding: 16px; } }
)CSS";

}  // namespace

void write_report(const ReportInput& input, std::ostream& output) {
    const IRProgram& original = *input.original;
    const bool optimized = input.optimization != nullptr;
    const IRProgram& final_program = optimized ? input.optimization->program : original;
    const PipelineDag& final_dag = optimized ? *input.optimized_dag : *input.original_dag;
    const std::map<std::string, std::size_t> colours = dataset_colours(original);
    const std::string name = input.pipeline->name;

    output << "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
           << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
           << "<title>" << escape(name) << " \xC2\xB7 PipeLang report</title>\n"
           << "<style>" << kStyle << "</style>\n</head>\n<body>\n<main>\n";

    output << "<header><h1>" << escape(name) << "</h1><p>PipeLang compilation report for <code>"
           << escape(input.source_path) << "</code></p></header>\n";

    // ------------------------------------------------------------ summary
    output << "<div class=\"stats\">\n";
    write_stat("statements", std::to_string(input.pipeline->statements.size()), output);
    write_stat("datasets", std::to_string(dataset_order(original).size()), output);
    write_stat(optimized ? "IR instructions (before \xE2\x86\x92 after)" : "IR instructions",
               optimized ? std::to_string(original.size()) + " \xE2\x86\x92 " +
                               std::to_string(final_program.size())
                         : std::to_string(original.size()),
               output);
    write_stat("DAG edges", std::to_string(final_dag.edges().size()), output);
    write_stat("execution levels", std::to_string(final_dag.execution_levels().size()), output);
    if (input.execution != nullptr) {
        std::size_t rows_out = 0;
        for (const DatasetOutput& dataset : input.execution->outputs) {
            rows_out += dataset.table.rows.size();
        }
        write_stat("output rows", std::to_string(rows_out), output);
        write_stat("run time", fixed(input.execution->total_ms, 2) + " ms", output);
        write_stat("worker threads", std::to_string(input.execution->workers), output);
    }
    output << "</div>\n";

    // ------------------------------------------------------------- source
    output << "<section><h2>Pipeline source</h2><p class=\"lead\">The program as written; "
              "line numbers match every diagnostic and optimization note.</p>"
           << "<pre class=\"code source\">";
    std::istringstream lines(input.source_text);
    std::string line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        output << "<span>" << escape(line) << "</span>";
    }
    output << "</pre>\n";
    for (const Diagnostic& warning : input.warnings) {
        output << "<div class=\"warning\"><strong>warning</strong> at line " << warning.line
               << ": " << escape(warning.message) << "</div>\n";
    }
    output << "</section>\n";

    // ---------------------------------------------------------------- DAG
    const char* legend =
        "<div class=\"legend\"><span class=\"lg-io\">input / output</span>"
        "<span class=\"lg-clean\">clean</span><span class=\"lg-filter\">filter</span>"
        "<span class=\"lg-transform\">transform</span><span class=\"lg-order\">sort / group</span>"
        "<span class=\"lg-aggregate\">aggregate</span></div>\n";
    output << "<section><h2>Dependency DAG</h2><p class=\"lead\">Each column is a dataset and "
              "each row an execution level: nodes in the same row have no dependency on each "
              "other and can run in parallel.</p>\n";
    if (optimized) {
        // Side by side for narrow graphs; wider ones are stacked to stay legible.
        const bool side_by_side = dataset_order(original).size() <= 2;
        output << "<div class=\"" << (side_by_side ? "grid-2" : "stacked")
               << "\"><div><h3>Before optimization ("
               << original.size() << " nodes)</h3><div class=\"scroll\">";
        write_dag_svg(original, *input.original_dag, colours, output);
        output << "</div></div><div><h3>After optimization (" << final_program.size()
               << " nodes)</h3><div class=\"scroll\">";
        write_dag_svg(final_program, final_dag, colours, output);
        output << "</div></div></div>\n";
    } else {
        output << "<div class=\"scroll\">";
        write_dag_svg(original, final_dag, colours, output);
        output << "</div>\n";
    }
    output << legend << "</section>\n";

    // ------------------------------------------------------- optimization
    if (optimized) {
        const auto& steps = input.optimization->steps;
        output << "<section><h2>Optimization</h2><p class=\"lead\">" << steps.size()
               << (steps.size() == 1 ? " optimization step" : " optimization steps")
               << ". Every pass keeps the pipeline's output identical.</p>\n";
        if (steps.empty()) {
            output << "<p class=\"muted\">The program was already optimal for these passes.</p>\n";
        } else {
            output << "<ol class=\"steps\">\n";
            for (const OptimizationStep& step : steps) {
                output << "<li><span class=\"pass\">" << escape(step.pass)
                       << "</span><span class=\"line-ref\">line " << step.line << "</span>"
                       << escape(step.message) << "</li>\n";
            }
            output << "</ol>\n";
        }
        output << "<div class=\"grid-2\"><div><h3>IR before</h3>";
        write_ir_listing(original, output);
        output << "</div><div><h3>IR after</h3>";
        write_ir_listing(final_program, output);
        output << "</div></div></section>\n";
    } else {
        output << "<section><h2>Intermediate representation</h2>";
        write_ir_listing(original, output);
        output << "</section>\n";
    }

    // ---------------------------------------------------------- execution
    if (input.execution != nullptr) {
        const ExecutionResult& execution = *input.execution;
        output << "<section><h2>Execution timeline</h2><p class=\"lead\">"
               << execution.workers << (execution.workers == 1 ? " worker thread" : " worker threads")
               << "; each bar is one DAG node, coloured by dataset. Inputs were loaded in "
               << fixed(input.inputs != nullptr ? input.inputs->load_ms : 0.0, 2)
               << " ms before the run started.</p>\n";
        write_timeline_svg(final_program, execution, colours, output);
        output << "<h3>Node timings</h3><div class=\"table-wrap\"><table><thead><tr>"
                  "<th>Node</th><th>Operation</th><th>Dataset</th><th>Line</th>"
                  "<th>Rows in</th><th>Rows out</th><th>Start (ms)</th><th>Time (ms)</th>"
                  "<th>Worker</th></tr></thead><tbody>\n";
        for (const NodeRun& run : execution.runs) {
            const IRInstruction& instruction = final_program[run.instruction];
            output << "<tr><td class=\"num\">" << run.instruction << "</td><td>"
                   << ir_opcode_name(instruction.opcode) << ' '
                   << "<span class=\"muted mono\">" << escape(instruction_detail(instruction))
                   << "</span></td><td>" << escape(instruction.dataset) << "</td><td class=\"num\">"
                   << instruction.line << "</td><td class=\"num\">" << run.rows_in
                   << "</td><td class=\"num\">" << run.rows_out << "</td><td class=\"num\">"
                   << fixed(run.start_ms, 3) << "</td><td class=\"num\">"
                   << fixed(run.end_ms - run.start_ms, 3) << "</td><td class=\"num\">"
                   << run.worker << "</td></tr>\n";
        }
        output << "</tbody></table></div>\n";
        if (input.inputs != nullptr) {
            output << "<h3>Input files</h3><div class=\"table-wrap\"><table><thead><tr>"
                      "<th>Dataset</th><th>File</th><th>Rows</th><th>Columns</th></tr></thead><tbody>\n";
            for (const auto& [dataset, summary] : input.inputs->summaries) {
                output << "<tr><td>" << escape(dataset) << "</td><td class=\"mono\">"
                       << escape(summary.file) << "</td><td class=\"num\">" << summary.rows
                       << "</td><td>";
                for (std::size_t column = 0; column < summary.columns.size(); ++column) {
                    if (column > 0) output << ", ";
                    output << escape(summary.columns[column].name) << " <span class=\"muted\">"
                           << column_type_name(summary.columns[column].type) << "</span>";
                }
                output << "</td></tr>\n";
            }
            output << "</tbody></table></div>\n";
        }
        output << "</section>\n";

        output << "<section><h2>Results</h2><p class=\"lead\">The output datasets, in program "
                  "order.</p>\n";
        for (const DatasetOutput& dataset : execution.outputs) {
            output << "<h3>" << escape(dataset.dataset) << " <span class=\"muted\">("
                   << dataset.table.rows.size()
                   << (dataset.table.rows.size() == 1 ? " row" : " rows");
            if (!dataset.file.empty()) output << ", written to " << escape(dataset.file);
            output << ")</span></h3>\n";
            write_table(dataset.table, 50, output);
        }
        output << "</section>\n";
    }

    output << "<footer>Generated by pipelangc \xC2\xB7 PipeLang compiler</footer>\n"
           << "</main>\n</body>\n</html>\n";
}

}  // namespace pipelang
