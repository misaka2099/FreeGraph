#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;

vector<int> arr;
vector<int> cmds;
vector<int> tmp;
void merge_sorted_arrs(vector<int>& arr, vector<int>& cmds, vector<int>& tmp)
{
    int i = 0, j = 0, k = 0;
    while (i < arr.size() && j < cmds.size())
    {
        if (arr[i] < cmds[j])
        {
            tmp[k++] = arr[i++];
        }
        else
        {
            tmp[k++] = cmds[j++];
        }
    }
    while (i < arr.size())
    {
        tmp[k++] = arr[i++];
    }
    while (j < cmds.size())
    {
        tmp[k++] = cmds[j++];
    }
}
int main()
{
    for (int i = 0 ; i < 100 ; i++)
    {
        arr.push_back(rand() % 1000);
    }
    sort(arr.begin(), arr.end());
    for (int i = 0 ; i < 10 ; i++)
    {
        cmds.push_back(rand() % 1000);
    }
    sort(cmds.begin(), cmds.end());
    cout << "ORIGINAL ARR: " << endl;
    for (int i = 0 ; i < arr.size() ; i++)
    {
        cout << arr[i] << " ";
    }
    cout << endl;
    cout << "ORIGINAL CMDS: " << endl;
    for (int i = 0 ; i < cmds.size() ; i++)
    {
        cout << cmds[i] << " ";
    }
    tmp.resize(arr.size() + cmds.size());
    merge_sorted_arrs(arr, cmds, tmp); 
    cout << "MERGED RESULT: " << endl;
    for (int i = 0 ; i < tmp.size() ; i++)
    {
        cout << tmp[i] << " ";
    }
    cout << endl;
    return 0;
}