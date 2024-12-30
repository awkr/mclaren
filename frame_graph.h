#pragma once

struct FrameGraph {};

void frame_graph_create(FrameGraph *frame_graph);
void frame_graph_destroy(FrameGraph *frame_graph);
void frame_graph_add_pass(FrameGraph *frame_graph);
void frame_graph_compile(FrameGraph *frame_graph);
void frame_graph_execute(FrameGraph *frame_graph);
