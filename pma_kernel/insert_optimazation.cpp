#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <random>

using namespace std;

#define SEGMENT_SIZE 64 
#define UNUSED_ICON -1

struct PMA_SEGMENT
{
    int data[SEGMENT_SIZE];
    PMA_SEGMENT()
    {
        for (int i = 0; i < SEGMENT_SIZE; ++i) data[i] = UNUSED_ICON;
    }
};

struct PMA {
    // segments[vertex_idx][segment_idx]
    vector<vector<PMA_SEGMENT>> segments; 
    
    void init(int num_vertices, int segments_per_vertex) {
        segments.resize(num_vertices);
        for(int i=0; i<num_vertices; ++i) {
            segments[i].resize(segments_per_vertex);
        }
    }
};

struct Stats {
    int read_count = 0;
    int write_count = 0;
} stats;

// Updated to handle 2D structure: vertex_idx, seg_idx
void read_seg(PMA& pma, int vertex_idx, int seg_idx, PMA_SEGMENT& buffer) {
    stats.read_count++;
    buffer = pma.segments[vertex_idx][seg_idx];
}

void write_seg(PMA& pma, int vertex_idx, int seg_idx, const PMA_SEGMENT& buffer) {
    stats.write_count++;
    pma.segments[vertex_idx][seg_idx] = buffer;
}

struct CachedSegment {
    int vertex_idx = -1;
    int seg_idx = -1;
    PMA_SEGMENT buffer;
    bool dirty = false;

    void load(PMA& pma, int v_idx, int s_idx) {
        if (vertex_idx != v_idx || seg_idx != s_idx) {
            if (dirty) flush(pma);
            read_seg(pma, v_idx, s_idx, buffer);
            vertex_idx = v_idx;
            seg_idx = s_idx;
            dirty = false;
        }
    }

    void flush(PMA& pma) {
        if (vertex_idx != -1 && seg_idx != -1 && dirty) {
            write_seg(pma, vertex_idx, seg_idx, buffer);
            dirty = false;
        }
    }
} cache;

void search_and_insert(PMA& pma, int vertex_idx, int num_segments, int dst) {
    int low_seg = 0;
    int high_seg = num_segments - 1;
    
    int target_seg = -1;
    
    // 1. Binary Search Phase
    while (low_seg <= high_seg) {
        int mid = low_seg + (high_seg - low_seg) / 2;
        cache.load(pma, vertex_idx, mid);
        const int* storage = cache.buffer.data;
        
        int minimum = INT_MAX;
        int maximum = INT_MIN;
        bool match_perfect = false;
        
        for (int j = 0; j < SEGMENT_SIZE; j++) {
            int val = storage[j];
            if (val != UNUSED_ICON) {
                maximum = max(maximum, val);
                minimum = min(minimum, val);
                if (val == dst) {
                    match_perfect = true;
                    return; 
                }
            }
        }
        
        if (match_perfect) return; // Edge already exists

        if (minimum == INT_MAX) {
             // Empty segment logic. 
             // Go left as empty segments (if padded) would be at end?
             // Or if using dynamic allocation they might be interspersed?
             // Assuming compact layout: empty means too far right.
             high_seg = mid - 1; 
             continue;
        }

        if (maximum < dst) {
            low_seg = mid + 1;
        } else if (minimum > dst) {
            high_seg = mid - 1;
        } else {
            target_seg = mid;
            break; 
        }
    }
    
    if (target_seg == -1) {
        // Not found in existing ranges, use low_seg as best guess
        target_seg = max(0, min(num_segments - 1, low_seg));
        // Simple bound check
        if (target_seg >= num_segments) target_seg = num_segments - 1;
    }
    
    // 2. Insert Phase (reuse/load correct segment)
    cache.load(pma, vertex_idx, target_seg);
    int* buffer = cache.buffer.data;
    
    int pos_in_seg = 0;
    while(pos_in_seg < SEGMENT_SIZE) {
        if (buffer[pos_in_seg] != UNUSED_ICON && buffer[pos_in_seg] < dst) {
            pos_in_seg++;
        } else {
            break;
        }
    }
    
    // Simple shift insert
    bool space_found = false;
    for (int k = pos_in_seg; k < SEGMENT_SIZE; k++) {
        if (buffer[k] == UNUSED_ICON) {
            space_found = true;
            for (int j = k; j > pos_in_seg; j--) {
                buffer[j] = buffer[j-1];
            }
            buffer[pos_in_seg] = dst;
            break;
        }
    }
    
    if (!space_found) {
        cout << "Warning: Segment " << target_seg << " of vertex " << vertex_idx << " full!" << endl;
    } else {
        cache.dirty = true;
    }
}

int main()
{
    PMA pma;
    pma.init(1, 10); // 1 Vertex (idx 0), 10 Segments
    
    // Initial Data for Vertex 0, Seg 0
    pma.segments[0][0].data[0] = 10; 
    pma.segments[0][0].data[1] = 20; 
    pma.segments[0][0].data[2] = 30;
    
    // Initial Data for Vertex 0, Seg 1
    pma.segments[0][1].data[0] = 40; 
    pma.segments[0][1].data[1] = 50;

    cout << "Initial Stats - Reads: " << stats.read_count << " Writes: " << stats.write_count << endl;

    // Insert 25 into Vertex 0
    // Should be found in Seg 0
    search_and_insert(pma, 0, 10, 25);
    cache.flush(pma); // Flush to commit

    cout << "After Insert 25 logic (Vertex 0, Seg 0):" << endl;
    for(int v : pma.segments[0][0].data) if (v!=-1) cout << v << " "; cout << endl;
    cout << "Stats - Reads: " << stats.read_count << " Writes: " << stats.write_count << endl;

    // Insert 45 into Vertex 0
    // Should be found in Seg 1
    // Search might check Seg 0 (cached), then Seg 1
    search_and_insert(pma, 0, 10, 45);
    cache.flush(pma);

    cout << "After Insert 45 logic (Vertex 0, Seg 1):" << endl;
    for(int v : pma.segments[0][1].data) if (v!=-1) cout << v << " "; cout << endl;
    cout << "Stats - Reads: " << stats.read_count << " Writes: " << stats.write_count << endl;

    return 0;
}
