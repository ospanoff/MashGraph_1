#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <vector>
#include <tuple>
#include <queue>
#include <cmath>
#include <algorithm>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using std::vector;
using std::tuple;
using std::tie;
using std::make_tuple;
using std::queue;

#include "io.h"
#include "matrix.h"

typedef tuple<uint, uint, uint, uint> Rect;

class MedianFilter
{
public:
    tuple<uint, uint, uint> operator () (const Image &m) const
    {
        uint size = 2 * radius + 1;
        vector <uint> r(size * size), g(size * size), b(size * size);
        for (uint i = 0; i < size; ++i) {
            for (uint j = 0; j < size; ++j) {
                // Tie is useful for taking elements from tuple
                tie(r[size * i + j], g[size * i + j], b[size * i + j]) = m(i, j);
            }
        }
        sort(r.begin(), r.end());
        sort(g.begin(), g.end());
        sort(b.begin(), b.end());

        return make_tuple(r[size*size / 2], g[size*size / 2], b[size*size / 2]);
    }
    // Radius of neighbourhoud, which is passed to that operator
    static const int radius = 2;
};

// Finds connected components
uint bfs(vector <vector <uint>> &used, const Image &binimg)
{
    uint m, n;
    uint k = 0;
    queue <tuple <uint, uint>> que;

    for (uint i = 0; i < used.size(); i++)
        for (uint j = 0; j < used[i].size(); j++) {
            if (!used[i][j]) {
                que.push(make_tuple(i, j));
                used[i][j] = ++k;
                while (!que.empty()) {
                    tie(m, n) = que.front();
                    que.pop();
                    if (m >= 1 && !used[m-1][n] && binimg(m,n) == binimg(m-1, n)) {
                        used[m-1][n] = k;
                        que.push(make_tuple(m-1, n));
                    }
                    if (m+1 < used.size() && !used[m+1][n] && binimg(m,n) == binimg(m+1, n)) {
                        used[m+1][n] = k;
                        que.push(make_tuple(m+1, n));
                    }
                    if (n >= 1 && !used[m][n-1] && binimg(m,n) == binimg(m, n-1)) {
                        used[m][n-1] = k;
                        que.push(make_tuple(m, n-1));
                    }
                    if (n+1 < used[i].size() && !used[m][n+1] && binimg(m,n) == binimg(m, n+1)) {
                        used[m][n+1] = k;
                        que.push(make_tuple(m, n+1));
                    }
                }
            }
        }
    return k;
}

// Makes img binary (!should add binarization by histogram!)
void make_binarization(Image &img)
{
    uint r, g, b;
    double threshold = 18;
    vector <uint> histogram(256);
    for (uint i = 0; i < img.n_rows; i++)
        for (uint j = 0; j < img.n_cols; j++) {
            tie(r, g, b) = img(i, j);
            uint lumin = 255;
            // RGB Luminance value = 0.299 R + 0.587 G + 0.114 B
            if (0.299 * r + 0.587 * g + 0.114 * b < threshold)
                lumin = 0;
            histogram[lumin]++;
            img(i, j) = make_tuple(lumin, lumin, lumin);
        }
}

vector <double> moment(const vector <vector <uint>> &used, const int m, const int n,
            const vector <uint> &avg_x, const vector <uint> &avg_y)
{
    vector <double> v(avg_x.size());

    for (long long i = 0; i < static_cast<long long>(used.size()); i++)
        for (long long j = 0; j < static_cast<long long>(used[0].size()); j++) {
            v[used[i][j]-1] += pow(j-avg_x[used[i][j]-1], m) * pow(i-avg_y[used[i][j]-1], n);
        }
    return v;
}

void count_geometrical_characteristics(const vector <vector <uint>> &used, vector <vector <tuple<uint, uint>>> &border,
                                       vector <uint> &area, vector <uint> &avg_x,
                                       vector <uint> &avg_y, vector <uint> &perim,
                                       vector <double> &elongation, vector <double> &theta)
{
    // Counting area, perimeter and mass center
    for (uint i = 0; i < used.size(); i++)
        for (uint j = 0; j < used[0].size(); j++) {
            area[used[i][j]-1]++;
            avg_x[used[i][j]-1] += j;
            avg_y[used[i][j]-1] += i;
            if (used[i][j] > 1 &&
                (used[i >= 1 ? i-1 : i][j]-1) * (used[i+1 < used.size() ? i+1 : i][j]-1) *
                (used[i][j >= 1 ? j-1 : j]-1) * (used[i][j+1 < used[0].size() ? j+1 : j]-1) == 0) {
                perim[used[i][j]-1]++;
                border[used[i][j]-1].push_back(make_tuple(i, j));
            }
        }

    for (uint i = 0; i < avg_x.size(); i++) {
        avg_x[i] /= area[i];
        avg_y[i] /= area[i];
    }

    // counting moments and elongations
    vector <double> moment20 = moment(used, 2, 0, avg_x, avg_y);
    vector <double> moment02 = moment(used, 0, 2, avg_x, avg_y);
    vector <double> moment11 = moment(used, 1, 1, avg_x, avg_y);

    for (uint i = 0; i < avg_x.size(); i++) {
        elongation[i] = (moment20[i] + moment02[i] + sqrt(pow((moment20[i] - moment02[i]), 2) + 4*pow(moment11[i], 2))) /
                        (moment20[i] + moment02[i] - sqrt(pow((moment20[i] - moment02[i]), 2) + 4*pow(moment11[i], 2)));
        // cout << i+1 << ": Moments: " << "(" << moment11[i] << ",\t" << moment20[i] << ",\t" << moment02[i] << ")" << endl;
    }
    cout << endl;

    // counting angles
    for (uint i = 0; i < avg_x.size(); i++)
        theta[i] = atan(2 * moment11[i] / (moment20[i] - moment02[i])) / 2;
}

void find_dirs(const vector <vector <tuple<uint, uint>>> &border,
               vector <uint> &avg_x, vector <uint> &avg_y, vector <double> &theta,
               vector <uint> &dir_x, vector <uint> &dir_y)
{
    vector <tuple <uint, uint>> ok;
    long long a, b;    
    double EPS = 0.03;
    for (uint i = 1; i < border.size(); i++) {
        // find direction dots
        for (uint j = 0; j < border[i].size(); j++) {
            tie(a, b) = border[i][j];
            double at;
            if (b == avg_x[i])
                at = M_PI/2;
            else
                at = atan(fabs((a - static_cast<long long>(avg_y[i]))) / fabs((b - static_cast<long long>(avg_x[i]))));
            
            if (at > M_PI/4)
                at -= M_PI/2;
            if (at < -M_PI/4)
                at += M_PI/2;

            if (fabs(at - theta[i]) < EPS || fabs(at + theta[i]) < EPS) {
                ok.push_back(make_tuple(a, b));
            }
        }

        // ...cont. of finding direction
        tie(a, b) = ok[0];
        long long max_x = b, max_y = a;
        for (uint k = 0; k < ok.size(); k++) {
            tie(a, b) = ok[k];
            double dx = fabs(b - avg_x[i]);
            double dy = fabs(a - avg_y[i]);
            double m_dx = fabs(max_x - avg_x[i]);
            double m_dy = fabs(max_y - avg_y[i]);
            if (dx * dx + dy * dy > m_dy * m_dy + m_dx * m_dx) {
                max_x = b;
                max_y = a;
            }
        }
        dir_x[i] = max_x;
        dir_y[i] = max_y;
        ok.clear();
    }
}

void find_way(Image &img, const vector <vector <uint>> &used,
              vector <uint> &avg_x, vector <uint> &avg_y, uint i,
              vector <uint> &dir_x, vector <uint> &dir_y, vector <uint> &perim,
              vector <uint> &area, vector <double> &elongation/*, vector <Rect> &path*/)
{
    cout << i+1 << ": " << elongation[i] << " ";
    if (perim[i]*perim[i] / area[i] > 17 || perim[i]*perim[i] / area[i] < 14 ||
        elongation[i] < 3.5 || elongation[i] > 4.15)
        return;
    long long l, dx, dy;
    long long xr = abs(dir_x[i] - avg_x[i]);
    long long yr = abs(dir_y[i] - avg_y[i]);
    if (xr > yr)
        l = xr;
    else
        l = yr;
    long long px = (avg_x[i] << 12) + (1 << 11);
    long long py = (avg_y[i] << 12) + (1 << 11);
    long long ex = (dir_x[i] << 12) + (1 << 11);
    long long ey = (dir_y[i] << 12) + (1 << 11);
    if (l != 0) {
        dx = (ex - px) / l;
        dy = (ey - py) / l;
    } else {
        dx = 0;
        dy = 0;
    }

    while ((py >> 12) >= 0 && (px >> 12) >= 0 &&
           (py >> 12) < static_cast<long long>(img.n_rows) && (px >> 12) < static_cast<long long>(img.n_cols) &&
           (used[py >> 12][px >> 12] == i+1 || used[py >> 12][px >> 12] == 1)) {
        img(py >> 12, px >> 12) = make_tuple(255, 0, 255);
        px += dx;
        py += dy;
    }

    cout << dir_y[i] << " " << dir_x[i] << "; " << endl;

    find_way(img, used, avg_x, avg_y, used[py >> 12][px >> 12] - 1, dir_x, dir_y, perim, area, elongation);
}

uint find_red_ptr(const Image &img, const vector <vector <uint>> &used)
{
    uint r, g, b, pix = 0;
    for (uint i = 1; i < img.n_rows-1; i++)
        for (uint j = 1; j < img.n_cols-1; j++) {
            pix = 0;
            for (uint k = i-1; k <= i+1; k++)
                for (uint l = j-1; l <= j+1; l++) {
                    tie(r, g, b) = img(k, l);
                    if (r > 230 && g < 70 && b < 70)
                        pix++;
                }
            if (pix >= 7)
                return used[i][j];
        }
    return 0;
}

tuple<vector<Rect>, Image>
find_treasure(const Image& in)
{
    // Base: return Rect of treasure only
    // Bonus: return Rects of arrows and then treasure Rect

    auto path = vector<Rect>();
        
    Image binimg = in.deep_copy();
    make_binarization(binimg);

    binimg = binimg.unary_map(MedianFilter());

    vector <vector <uint>> used(in.n_rows);
    for (auto &rows : used)
        rows.resize(in.n_cols);

    uint k = bfs(used, binimg);
    cout << k << endl;

    vector <uint> area(k), avg_x(k), avg_y(k), perim(k);
    vector <double> elongation(k);
    vector <double> theta(k);
    vector <vector <tuple <uint, uint>>> border(k);
    count_geometrical_characteristics(used, border, area, avg_x, avg_y, perim, elongation, theta);
    
    /*for (uint i = 0; i < used.size(); i++)
        for (uint j = 0; j < used[0].size(); j++)
            if (area[used[i][j]-1] < 350 || area[used[i][j]-1] > 4800)
                used[i][j] = 1;
    */

    for (uint i = 0; i < k; i++) {
        cout << i+1 << ": " << " \tPerim: " << perim[i] << " \tArea: " << area[i] <<
        " \tCompact: " << perim[i]*perim[i] / area[i] << " \tElongation: " << elongation[i] <<
        " \tAngles: " << theta[i] << " \tAvg point (" << avg_x[i] << ", " << avg_y[i] << ")" << endl;
    }

    Image img = in.deep_copy();
    /*for (uint i = 0; i < in.n_rows; i++)
        for (uint j = 0; j < in.n_cols; j++) {
            img(i, j) = make_tuple(255-5*used[i][j], 255-15*used[i][j], 255-30*used[i][j]);
        }
    
    int a, b;
    for (uint i = 0; i < border.size(); ++i) {
        for (uint j = 0; j < border[i].size(); ++j) {
            tie(a, b) = border[i][j];
            img(a, b) = make_tuple(0, 0, 0);
        }
    }
    */
    vector <uint> dir_x(k), dir_y(k);
    find_dirs(border, avg_x, avg_y, theta, dir_x, dir_y);
    find_way(img, used, avg_x, avg_y, find_red_ptr(in, used) - 1, dir_x, dir_y, perim, area, elongation/*, path*/);

    // img = binimg;
    /*for (uint i = 0; i < avg_x.size(); i++)
        img(avg_y[i], avg_x[i]) = make_tuple(255, 0, 0);
*/

    /*for (uint i = 0; i < used.size(); i++) {
        for (uint j = 0; j < used[0].size(); j++)
            cout << used[i][j];
        cout << endl;
    }*/

    return make_tuple(path, img/*in.deep_copy()*/);
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        cout << "Usage: " << endl << argv[0]
             << " <in_image.bmp> <out_image.bmp> <out_path.txt>" << endl;
        return 0;
    }

    try {
        Image src_image = load_image(argv[1]);
        ofstream fout(argv[3]);

        vector<Rect> path;
        Image dst_image;
        tie(path, dst_image) = find_treasure(src_image);
        save_image(dst_image, argv[2]);

        uint x, y, width, height;
        for (const auto &obj : path)
        {
            tie(x, y, width, height) = obj;
            fout << x << " " << y << " " << width << " " << height << endl;
        }

    } catch (const string &s) {
        cerr << "Error: " << s << endl;
        return 1;
    }
}
