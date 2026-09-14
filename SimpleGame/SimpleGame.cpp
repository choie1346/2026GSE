/* Copyright 2022 Lee Taek Hee (Tech University of Korea).
   Distributed under the What The Hell License, without warranty. */
#include "stdafx.h"
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "Dependencies/glew.h"
#include "Dependencies/freeglut.h"
#include "Renderer.h"

struct V { float x, y; };
struct C { float r, g, b, a; };
struct Person { const wchar_t* name; V p; C coat; const wchar_t* lines[3]; int talks; };
struct Building { V p; C roof; };
struct Tree { V p; float size; };
struct Pillar { V p; float height; };
struct Herb { V p; bool picked; };
const int MapSize = 36;
const V Lake = { 27, 24 }, Ruin = { 26, 9 }, Bell = { 18, 17 }, Well = { 16, 18 };
Renderer* renderer = nullptr;
int width = 1280, height = 800, stage = 0, collected = 0;
float timeNow = 0, lastTime = 0, walk = 0, messageTime = 0;
V player = { 18, 19 }, camera = {}, facing = { 0, -1 };
bool keys[256] = {}, moving = false;
std::wstring message;
std::vector<Person> people;
std::vector<Building> buildings;
std::vector<Tree> trees;
std::vector<Pillar> pillars;
std::vector<Herb> herbs;
float dist(V a, V b) { return std::hypot(a.x - b.x, a.y - b.y); }
V iso(V p) { return { (p.x - p.y) * 42, -(p.x + p.y) * 21 }; }
V screen(V p) { V s = iso(p); return { s.x - camera.x,s.y - camera.y }; }
C shade(C c, float s) { return { c.r * s,c.g * s,c.b * s,c.a }; }
void rect(float x, float y, float w, float h, C c) {
    renderer->DrawSolidQuad(x - w / 2, y - h / 2, x + w / 2, y - h / 2, x + w / 2, y + h / 2, x - w / 2, y + h / 2, c.r, c.g, c.b, c.a);
}
void tri(V a, V b, V d, C c) { renderer->DrawSolidTriangle(a.x, a.y, b.x, b.y, d.x, d.y, c.r, c.g, c.b, c.a); }
void ellipse(float x, float y, float rx, float ry, C c) { renderer->DrawSolidEllipse(x, y, rx, ry, 32, c.r, c.g, c.b, c.a); }
void diamond(V p, float w, float h, C c) {
    renderer->DrawSolidQuad(p.x, p.y + h / 2, p.x + w / 2, p.y, p.x, p.y - h / 2, p.x - w / 2, p.y, c.r, c.g, c.b, c.a);
}
bool visible(V p, float margin = 150) { V s = screen(p); return std::abs(s.x) < width / 2 + margin && std::abs(s.y) < height / 2 + margin; }
void text(float x, float y, const std::wstring& s, C c = { .92f,.94f,.89f,1 }) { renderer->DrawText(x, y, s.c_str(), c.r, c.g, c.b, c.a); }
void say(const std::wstring& s) { message = s; messageTime = 10; }
bool lake(V p) { return std::pow((p.x - Lake.x) / 5.8f, 2) + std::pow((p.y - Lake.y) / 4.5f, 2) < 1; }
bool ruin(V p) { return dist(p, Ruin) < 3.5f; }
bool road(V p) {
    return (p.x > 15 && p.x < 21 && p.y>15 && p.y < 21) ||
        (std::abs(p.y - 18) < .85f && p.x > 9 && p.x < 26) ||
        (std::abs(p.x - 18) < .8f && p.y > 9 && p.y < 25) ||
        (std::abs(p.y - 10) < .8f && p.x > 17 && p.x < 28) ||
        (std::abs(p.x - 11) < .8f && p.y > 12 && p.y < 19);
}
bool blocked(V p) {
    if (p.x < .5f || p.y < .5f || p.x>35.5f || p.y>35.5f || lake(p) || dist(p, Well) < .6f) return true;
    for (const auto& b : buildings) if (std::abs(p.x - b.p.x) < .85f && std::abs(p.y - b.p.y) < .85f) return true;
    for (const auto& t : trees) if (dist(p, t.p) < .28f) return true;
    for (const auto& t : pillars) if (dist(p, t.p) < .42f) return true;
    return false;
}
void reset() {
    stage = collected = 0; player = { 18,19 }; walk = 0; moving = false;
    std::fill(keys, keys + 256, false);
    buildings = { {{14,15},{.52f,.22f,.19f,1}},{{20,14},{.29f,.38f,.43f,1}},
        {{14,21},{.45f,.24f,.32f,1}},{{22,17},{.48f,.29f,.17f,1}},
        {{20,23},{.28f,.38f,.34f,1}},{{16,24},{.52f,.24f,.19f,1}},
        {{12,17},{.27f,.34f,.43f,1}},{{24,15},{.46f,.26f,.24f,1}},{{15,12},{.35f,.37f,.39f,1}} };
    people = {
        {L"마라 촌장",{17,16},{.47f,.35f,.54f,1},{L"왔구나. 저녁 종을 울려야 하는데 약초가 모자라네. 잠깐 도와주겠니?",L"숲 입구, 호숫가, 옛 예배당에 하나씩 있단다. 세 포기면 충분해.",L"수고했다. …아니, 방금 누구와 함께 온 것 같아서."},0},
        {L"어부 토벤",{23,19},{.23f,.46f,.54f,1},{L"오늘은 빈 그물만 세 번 건졌어. 물고기들도 종소리를 기다리나 봐.",L"호숫가 얕은 돌밭을 찾아봐. 푸른 꽃이 하나 보일 거야.",L"물이 잠잠해졌네. 내일은 배를 띄울 수 있겠어."},0},
        {L"빵집 주인 일제",{13,19},{.72f,.46f,.29f,1},{L"막 구운 빵 냄새가 나지? 늘 하나씩 남는데, 오늘도 손이 먼저 움직였네.",L"돌아오는 길에 들러. 따뜻한 걸로 남겨 둘게.",L"빵 하나를 따로 뒀는데… 누구 몫이었더라?"},0},
        {L"경비병 렌",{19,15.5f},{.35f,.41f,.47f,1},{L"북쪽 길은 열려 있어. 무너진 돌담에는 가까이 가지 마.",L"예배당은 북쪽 갈림길에서 동쪽으로. 기둥 사이로 들어가면 돼.",L"출입 장부가 한 장 비었군. 아침에는 분명 적어 뒀는데."},0},
        {L"니아",{18,21},{.77f,.43f,.49f,1},{L"우리 집 식탁에는 의자가 하나 더 있어. 아무도 거기 앉으면 안 된대.",L"약초 찾는 거야? 반짝이는 꽃이면 나도 봤어!",L"엄마가 의자는 원래 세 개였대. 내가 잘못 셌나?"},0},
        {L"약초꾼 오린",{11,18},{.31f,.53f,.35f,1},{L"푸른 꽃잎에 은빛 줄기가 있으면 달빛풀이지. 다른 풀과 헷갈리진 않을 거야.",L"뿌리는 남겨 두렴. 내년에도 꽃을 봐야 하니까.",L"손을 씻어도 향이 남을 거야. 하루쯤 지나면 괜찮아."},0},
        {L"직조공 셀라",{15,22},{.57f,.43f,.63f,1},{L"옷 안쪽에 이름을 수놓는 중이야. 잃어버려도 주인을 찾을 수 있게.",L"숲 입구에 내가 묶어 둔 붉은 천이 있어. 그쪽으로 가 봐.",L"이 옷의 주인이 누구였지? 이름까지 지워졌네."},0},
        {L"순례자 에다",{21,20},{.66f,.59f,.34f,1},{L"하룻밤 묵으려다 사흘째야. 이 마을, 이상하게 발길이 안 떨어져.",L"옛 예배당에는 이름을 새긴 돌이 있어. 누가 모두 긁어 놓았더군.",L"내일은 떠나려고. 내가 어디서 왔는지 아직 기억할 때."},0},
        {L"방앗간지기 돈",{19,24},{.48f,.43f,.34f,1},{L"밀가루 배달이 하나 남았어. 주소가 없어서 종일 들고 다니네.",L"광장으로 돌아오면 종부터 찾아. 해가 금방 넘어갈 거야.",L"배달은 다 끝났어. 빈 자루를 왜 들고 있었는지 모르겠군."},0},
        {L"도예가 주리",{13,15.8f},{.61f,.34f,.27f,1},{L"호수 진흙으로 빚으면 유약이 푸르게 나와. 다른 곳 흙으로는 안 돼.",L"옛 예배당 바닥도 같은 흙으로 구웠대. 지금은 이끼투성이지만.",L"가마 속 그릇이 하나 깨졌어. 종이 울릴 때였나."},0},
        {L"마구간지기 카데",{23,16.5f},{.39f,.46f,.34f,1},{L"말들이 북쪽 길만 보면 멈춰 서. 예전에는 잘 다녔는데.",L"돌아올 땐 광장 쪽으로 와. 숲 안쪽은 길이 끊겼어.",L"말들이 이제 조용하네. 오늘은 푹 자겠어."},0},
        {L"정원사 미라",{16,25.5f},{.43f,.57f,.35f,1},{L"누가 화단에 발자국을 남겼어. 아이들한테 물어도 아니래.",L"달빛풀은 밟아도 다시 일어나더라. 그래도 조심히 다뤄 줘.",L"발자국이 없어졌네. 비도 안 왔는데."},0},
        {L"나룻배꾼 보라",{21,24},{.25f,.43f,.51f,1},{L"나루는 문 닫았어. 돌아오는 배가 없어서 말이야.",L"호수는 돌아서 가. 얕아 보여도 바닥이 갑자기 꺼져.",L"배를 기다렸다고? 나는 오늘 하루 종일 그물을 손봤는데."},0},
        {L"이야기꾼 유나",{16.7f,20.5f},{.64f,.32f,.36f,1},{L"옛날에는 저 종을 두 사람이 함께 울렸대. 혼자서는 너무 무거웠거든.",L"어느 날부터 혼자 울릴 수 있게 됐지. 그 뒤 이야기는 아무도 몰라.",L"종은 원래 한 사람이 울리는 거야. 내가 다른 말을 했니?"},0},
        {L"북문지기 하네",{18,11.5f},{.35f,.42f,.44f,1},{L"예배당에 간다면 낮에 다녀와. 밤에는 돌계단이 잘 안 보여.",L"입구의 무너진 기둥만 피하면 돼. 안쪽은 아직 걸을 만해.",L"예배당 쪽에서 불빛을 봤어. 네가 켜 두고 온 건가?"},0},
        {L"숲지기 소리",{10,13},{.26f,.47f,.32f,1},{L"올해는 안개가 낮게 깔리네. 길 가장자리만 보고 걸어.",L"붉은 천 옆에 달빛풀이 있어. 뿌리까지 뽑지는 말고.",L"숲이 갑자기 조용해졌어. 늘 듣던 소리가 하나 빠진 것 같아."},0},
        {L"조사자 이벤",{27.8f,11},{.46f,.40f,.34f,1},{L"묘비인 줄 알았는데 마을 사람들 이름이 새겨져 있어. 살아 있는 사람들 말이야.",L"돌에 난 자국을 봐. 오래된 상처 위에 새로 긁은 흔적이 있어.",L"내 수첩에도 빈 줄이 생겼어. 잉크가 번진 건 아닌데."},0},
        {L"호숫가 노인 가란",{29,29},{.49f,.47f,.41f,1},{L"저녁 종이 울리면 모두 집으로 돌아가지. 늘 한 사람만 빼고.",L"누가 남았는지는 묻지 말게. 대답할 수 있는 사람이 없어.",L"또 저녁이 왔구먼. 자네는 집으로 돌아가게."},0}
    };
    herbs = { {{10.8f,12.2f},false},{{23,20.2f},false},{{26,9},false} };
    pillars = { {{24.7f,8.5f},65},{{27.3f,8.5f},49},{{24.7f,10.5f},36},{{27.3f,10.5f},58},{{26,7.6f},18} };
    trees.clear();
    for (int x = 2; x < 35; x += 2) for (int y = 2; y < 35; y += 2) {
        V p = { x + .35f * std::sin(float(y * 7)),y + .35f * std::cos(float(x * 5)) };
        if ((x < 12 || y < 7 || x>31 || y>31) && !road(p) && !lake(p) && !ruin(p)) {
            bool clear = true;
            for (const auto& h : herbs) if (dist(p, h.p) < 1.3f) clear = false;
            for (const auto& n : people) if (dist(p, n.p) < 1.3f) clear = false;
            if (clear) trees.push_back({ p,.85f + float((x + y) % 5) * .1f });
        }
    }
    camera = iso(player); camera.y -= 20;
    say(L"마라 촌장이 광장에서 기다리고 있습니다.");
}
int nearPerson() {
    int best = -1; float d = 1.2f;
    for (int i = 0; i < (int)people.size(); ++i) if (dist(player, people[i].p) < d) { d = dist(player, people[i].p); best = i; }
    return best;
}
int nearHerb() { for (int i = 0; i < (int)herbs.size(); ++i) if (!herbs[i].picked && dist(player, herbs[i].p) < 1.1f) return i; return -1; }
void interact() {
    int h = nearHerb(), n = nearPerson();
    if (stage == 1 && h >= 0) { herbs[h].picked = true; ++collected; if (collected == 3) stage = 2; say(collected == 3 ? L"세 포기를 모두 모았습니다. 광장의 종으로 돌아가세요." : L"달빛풀을 조심스럽게 꺾었습니다. 손에 풀 향기가 남습니다."); }
    else if (stage == 2 && dist(player, Bell) < 1.2f) { stage = 3; say(L"종이 한 번 울렸다. 잠시 뒤, 누군가의 이름이 떠오르지 않았다."); }
    else if (n >= 0) {
        Person& p = people[n];
        int line = stage == 3 ? 2 : (stage == 0 ? 0 : 1);
        if (p.talks++ % 2 && stage != 3) line = 1 - line;
        say(std::wstring(p.name) + L"\n" + p.lines[line]);
        if (n == 0 && stage == 0) stage = 1;
    }
    else if (dist(player, Ruin) < 2) say(L"이름이 지워진 기념비\n돌을 긁어낸 자리가 유난히 희다. 아직 돌가루가 남아 있다.");
    else if (dist(player, Bell) < 1.2f) say(L"종 아래에 약초를 놓았던 자국이 남아 있다. 매년 같은 자리에 놓은 듯하다.");
}
void shadow(V p, float radius, float tall) {
    V s = screen(p);
    for (int i = 7; i >= 1; --i) ellipse(s.x + tall * .26f, s.y - tall * .12f, radius + i * 1.7f, radius * .32f + i * .7f, { .025f,.035f,.04f,.035f });
    ellipse(s.x, s.y, radius * .65f, radius * .22f, { .025f,.035f,.04f,.20f });
}
void box(V p, float w, float d, float h, C c) {
    V s = screen(p);
    renderer->SetMaterial(1);
    renderer->DrawSolidQuad(s.x - w / 2, s.y, s.x, s.y - d / 2, s.x, s.y - d / 2 + h, s.x - w / 2, s.y + h, c.r * .72f, c.g * .72f, c.b * .72f, c.a);
    renderer->DrawSolidQuad(s.x, s.y - d / 2, s.x + w / 2, s.y, s.x + w / 2, s.y + h, s.x, s.y - d / 2 + h, c.r, c.g, c.b, c.a);
    diamond({ s.x,s.y + h }, w, d, shade(c, 1.1f));
    renderer->SetMaterial(0);
}
void ground() {
    for (int x = 0; x < MapSize; ++x) for (int y = 0; y < MapSize; ++y) {
        V p = { x + .5f,y + .5f }; if (!visible(p, 90)) continue;
        C c = road(p) ? C{ .52f,.49f,.39f,1 } : (ruin(p) ? C{ .43f,.46f,.41f,1 } : C{ .26f,.43f,.29f,1 });
        if (x < 12 || y < 7) if (!road(p)) c = { .18f,.33f,.24f,1 };
        renderer->SetMaterial(road(p) || ruin(p) ? 1 : 2);
        diamond(screen(p), 84.5f, 42.5f, c);
    }
    renderer->SetMaterial(0);
    // A continuous projected shoreline avoids a staircase of water tiles.
    const int segments = 128;
    V center = screen(Lake);
    for (int i = 0; i < segments; ++i) {
        float a = i * 6.2831853f / segments, b = (i + 1) * 6.2831853f / segments;
        V pa = screen({ Lake.x + 6.05f * std::cos(a),Lake.y + 4.75f * std::sin(a) });
        V pb = screen({ Lake.x + 6.05f * std::cos(b),Lake.y + 4.75f * std::sin(b) });
        tri(center, pa, pb, { .47f,.53f,.43f,1 });
    }
    renderer->SetMaterial(3);
    for (int i = 0; i < segments; ++i) {
        float a = i * 6.2831853f / segments, b = (i + 1) * 6.2831853f / segments;
        tri(center, screen({ Lake.x + 5.8f * std::cos(a),Lake.y + 4.5f * std::sin(a) }), screen({ Lake.x + 5.8f * std::cos(b),Lake.y + 4.5f * std::sin(b) }), { .16f,.44f,.51f,1 });
    }
    renderer->SetMaterial(0);
    for (const auto& b : buildings) if (visible(b.p)) shadow(b.p, 47, 70);
    for (const auto& t : trees) if (visible(t.p)) shadow(t.p, 25 * t.size, 85 * t.size);
    for (const auto& p : pillars) if (visible(p.p)) shadow(p.p, 17, p.height);
    for (const auto& n : people) if (visible(n.p)) shadow(n.p, 12, 30);
    shadow(player, 13, 35); shadow(Bell, 24, 60); shadow(Well, 21, 20);
}
void house(const Building& b) {
    V s = screen(b.p); box(b.p, 94, 47, 64, { .65f,.65f,.57f,1 });
    renderer->SetMaterial(4);
    tri({ s.x - 57,s.y + 59 }, { s.x,s.y + 108 }, { s.x + 57,s.y + 59 }, b.roof);
    tri({ s.x - 57,s.y + 59 }, { s.x,s.y + 35 }, { s.x + 57,s.y + 59 }, shade(b.roof, .7f));
    renderer->SetMaterial(0);
    rect(s.x, s.y + 15, 20, 34, { .23f,.22f,.19f,1 });
    for (int side = -1; side <= 1; side += 2) {
        rect(s.x + side * 31, s.y + 28, 18, 24, { .25f,.28f,.27f,1 });
        rect(s.x + side * 31, s.y + 29, 12, 17, { .91f,.76f,.43f,1 });
        rect(s.x + side * 31, s.y + 29, 2, 18, { .31f,.28f,.23f,1 });
    }
    for (int i = -1; i <= 1; ++i) rect(s.x + i * 44, s.y + 29, 4, 49, { .30f,.28f,.23f,1 });
    rect(s.x + 30, s.y + 88, 11, 24, { .40f,.41f,.38f,1 });
    for (int i = 0; i < 5; ++i) { float t = std::fmod(timeNow * .22f + i * .2f, 1.f); ellipse(s.x + 30 + t * 20, s.y + 104 + t * 48, 5 + t * 12, 5 + t * 8, { .83f,.86f,.84f,(1 - t) * .10f }); }
}
void tree(const Tree& t) {
    V s = screen(t.p); float z = t.size;
    rect(s.x, s.y + 22 * z, 9 * z, 46 * z, { .30f,.26f,.20f,1 });
    float sway = std::sin(timeNow * 1.3f + t.p.x) * 1.8f;
    for (int i = 0; i < 7; ++i) {
        float a = i * 2.399f;
        ellipse(s.x + std::cos(a) * 18 * z + sway, s.y + (63 + std::sin(a) * 17) * z, (24 - i % 3 * 2) * z, 24 * z, { .16f + i * .013f,.36f + i * .017f,.24f + i * .007f,1 });
    }
    if (t.p.x < 12 && t.p.y>10 && t.p.y < 14) rect(s.x, s.y + 25, 13, 4, { .75f,.22f,.25f,1 });
}
void character(V p, C coat, int id, bool hero = false) {
    V s = screen(p); float scale = id == 4 ? .8f : 1.f;
    float bob = hero && moving ? std::abs(std::sin(walk)) * 1.7f : std::sin(timeNow * 2 + id) * .7f;
    float step = hero && moving ? std::sin(walk) * 4 : 0;
    s.y += bob;
    C skin = { .77f - id % 3 * .035f,.60f - id % 3 * .025f,.47f - id % 3 * .02f,1 };
    for (int side = -1; side <= 1; side += 2) {
        ellipse(s.x + side * 5, s.y + 10 + side * step, 3.5f, 10, { .24f,.25f,.26f,1 });
        ellipse(s.x + side * 5, s.y + 2 + side * step, 5, 3, { .16f,.16f,.16f,1 });
    }
    ellipse(s.x, s.y + 27 * scale, 11, 17 * scale, shade(coat, .72f));
    ellipse(s.x - 2, s.y + 29 * scale, 8, 14 * scale, coat);
    rect(s.x, s.y + 23 * scale, 19, 3, { .40f,.31f,.20f,1 });
    for (int side = -1; side <= 1; side += 2) {
        ellipse(s.x + side * 12, s.y + 29 * scale - side * step * .3f, 4, 10, coat);
        ellipse(s.x + side * 12, s.y + 20 * scale - side * step * .3f, 3, 4, skin);
    }
    ellipse(s.x, s.y + 48 * scale, 9, 11, { .24f,.21f,.20f,1 });
    ellipse(s.x + 1, s.y + 46 * scale, 7.3f, 8.5f, skin);
    ellipse(s.x - 2, s.y + 53 * scale, 8, 5, { .30f + id % 3 * .07f,.24f,.20f,1 });
    float gaze = hero ? facing.x * 1.5f : 1;
    if (!hero || facing.y <= 0) for (int side = -1; side <= 1; side += 2) ellipse(s.x + gaze + side * 2.5f, s.y + 47 * scale, 1, 1.3f, { .13f,.15f,.16f,1 });
    if (id % 3 == 0) { ellipse(s.x, s.y + 55 * scale, 12, 3, shade(coat, .85f)); ellipse(s.x, s.y + 59 * scale, 8, 5, coat); }
    if (id % 3 == 1) rect(s.x + 15, s.y + 22, 3, 37, { .43f,.34f,.22f,1 });
    if (hero) { rect(s.x - 8, s.y + 32, 4, 20, { .77f,.66f,.37f,1 }); ellipse(s.x + 8, s.y + 22, 5, 6, { .44f,.30f,.20f,1 }); }
}
void drawHerb(const Herb& h) {
    if (h.picked) return; V s = screen(h.p); float bob = std::sin(timeNow * 2.5f) * 2;
    for (int i = 5; i > 0; --i) ellipse(s.x, s.y + 14, 7 + i * 3, 5 + i * 2, { .40f,.82f,.82f,.025f });
    rect(s.x, s.y + 8, 2, 16, { .30f,.61f,.39f,1 });
    for (int i = 0; i < 5; ++i) { float a = i * 1.2566f; ellipse(s.x + std::cos(a) * 5, s.y + 18 + bob + std::sin(a) * 4, 4, 4, { .68f,.91f,.91f,1 }); }
}
void objects() {
    struct Item { V p; int kind, index; }; std::vector<Item> list;
    for (int i = 0; i < (int)buildings.size(); ++i) list.push_back({ buildings[i].p,0,i });
    for (int i = 0; i < (int)trees.size(); ++i) list.push_back({ trees[i].p,1,i });
    for (int i = 0; i < (int)people.size(); ++i) list.push_back({ people[i].p,2,i });
    for (int i = 0; i < (int)pillars.size(); ++i) list.push_back({ pillars[i].p,3,i });
    for (int i = 0; i < (int)herbs.size(); ++i) list.push_back({ herbs[i].p,4,i });
    list.push_back({ player,5,0 }); list.push_back({ Bell,6,0 }); list.push_back({ Well,7,0 });
    std::stable_sort(list.begin(), list.end(), [](const Item& a, const Item& b) {return a.p.x + a.p.y < b.p.x + b.p.y; });
    for (const auto& o : list) {
        if (!visible(o.p)) continue; V s = screen(o.p);
        switch (o.kind) {
        case 0:house(buildings[o.index]); break;
        case 1:tree(trees[o.index]); break;
        case 2:character(o.p, people[o.index].coat, o.index); break;
        case 3:box(o.p, 25, 18, pillars[o.index].height, { .54f,.57f,.52f,1 }); ellipse(s.x - 5, s.y + 12, 8, 5, { .29f,.42f,.29f,.8f }); break;
        case 4:drawHerb(herbs[o.index]); break;
        case 5:character(player, { .30f,.46f,.67f,1 }, 20, true); break;
        case 6:
            box(Bell, 48, 25, 10, { .50f,.51f,.47f,1 });
            rect(s.x - 19, s.y + 34, 6, 63, { .34f,.29f,.22f,1 }); rect(s.x + 19, s.y + 34, 6, 63, { .34f,.29f,.22f,1 });
            rect(s.x, s.y + 65, 48, 6, { .39f,.33f,.24f,1 }); ellipse(s.x, s.y + 44, 14, 17, { .69f,.54f,.29f,1 });
            ellipse(s.x, s.y + 32, 18, 5, { .81f,.67f,.37f,1 }); break;
        case 7:box(Well, 40, 24, 20, { .51f,.55f,.54f,1 }); ellipse(s.x, s.y + 21, 14, 6, { .16f,.31f,.35f,1 }); break;
        }
    }
}
std::wstring wrap(const std::wstring& s, int columns) {
    std::wstring out; int n = 0;
    for (wchar_t ch : s) { int size = ch < 128 ? 1 : 2; if (ch == L'\n') n = 0; else if (n + size > columns) { out += L'\n'; n = 0; }out += ch; n += size; }
    return out;
}
void hud() {
    float left = -width * .5f + 24, top = height * .5f - 34;
    std::wstring objective = stage == 0 ? L"저녁 종  ·  마라 촌장과 대화" : stage == 1 ? L"달빛풀 모으기  " + std::to_wstring(collected) + L" / 3" : stage == 2 ? L"광장의 종에 달빛풀 놓기" : L"저녁 종  ·  완료";
    float headerWidth = std::min(530.f, float(width) - 48);
    rect(left + headerWidth / 2 - 8, top - 26, headerWidth, 90, { .07f,.10f,.10f,.86f });
    text(left, top, wrap(objective, std::max(12, int((headerWidth - 24) / 12))));
    text(left, top - 32, L"WASD 이동   E 대화·조사   R 재시작");
    const wchar_t* area = ruin(player) ? L"잊힌 예배당" : dist(player, Lake) < 8 ? L"고요한 호수" : player.x < 12 ? L"붉은 실 숲" : L"달빛마을";
    text(width * .5f - 220, width < 900 ? top - 110 : top, area);
    // Objective marker stays at the viewport edge when the target is off screen.
    V target = stage == 0 ? people[0].p : Bell;
    if (stage == 1) { float nearest = 1000; for (const auto& h : herbs) if (!h.picked && dist(player, h.p) < nearest) { target = h.p; nearest = dist(player, h.p); } }
    if (stage < 3) {
        V s = screen(target); float limitX = std::max(80.f, width * .5f - 55), limitY = std::max(80.f, height * .5f - 180);
        float factor = std::max(1.f, std::max(std::abs(s.x) / limitX, std::abs(s.y) / limitY)); s.x /= factor; s.y /= factor;
        diamond({ s.x,s.y + 70 }, 12, 18, { .97f,.80f,.39f,1 });
    }
    float panelWidth = std::min(900.f, float(width) - 48);
    if (messageTime > 0) {
        std::wstring lines = wrap(message, std::max(12, int((panelWidth - 40) / 12)));
        int rows = 1 + int(std::count(lines.begin(), lines.end(), L'\n'));
        float h = rows * 27.f + 28;
        rect(0, -height * .5f + 45 + h / 2, panelWidth, h, { .055f,.075f,.08f,.92f });
        text(-panelWidth / 2 + 20, -height * .5f + 45 + h - 27, lines);
    }
    int n = nearPerson();
    std::wstring prompt = stage == 1 && nearHerb() >= 0 ? L"E  달빛풀 줍기" : n >= 0 ? std::wstring(L"E  대화: ") + people[n].name : dist(player, Bell) < 1.2f ? L"E  종 살펴보기" : dist(player, Ruin) < 2 ? L"E  기념비 조사" : L"";
    text(-panelWidth / 2 + 20, -height * .5f + 16, prompt, { .96f,.82f,.50f,1 });
}
void render() {
    glClearColor(.13f, .22f, .20f, 1); renderer->SetWorld(camera.x, camera.y, timeNow); renderer->BeginFrame();
    ground(); objects(); renderer->EndFrame(timeNow); hud(); glutSwapBuffers();
}
void idle() {
    float now = glutGet(GLUT_ELAPSED_TIME) * .001f, dt = std::min(.04f, now - lastTime); lastTime = now; timeNow += dt; messageTime -= dt;
    // Screen-relative movement is transformed back to the isometric plane.
    float sx = float(keys['d']) - float(keys['a']), sy = float(keys['w']) - float(keys['s']);
    V v = { sx - sy,-sx - sy }; float len = std::hypot(v.x, v.y); V old = player;
    if (len > 0) { v.x /= len; v.y /= len; V p = { player.x + v.x * 3.8f * dt,player.y }; if (!blocked(p))player = p; p = { player.x,player.y + v.y * 3.8f * dt }; if (!blocked(p))player = p; facing = { sx,sy }; }
    moving = dist(old, player) > .0001f; if (moving)walk += dt * 10;
    camera = iso(player); camera.y -= 20; glutPostRedisplay();
}
void down(unsigned char key, int, int) {
    if (key == 27) { glutLeaveMainLoop(); return; }
    if (key >= 'A' && key <= 'Z')key += 32;
    if (keys[key])return; keys[key] = true;
    if (key == 'e')interact(); if (key == 'r')reset();
}
void up(unsigned char key, int, int) { if (key >= 'A' && key <= 'Z')key += 32; keys[key] = false; }
void resize(int w, int h) { width = std::max(1, w); height = std::max(1, h); if (renderer)renderer->SetWindowSize(width, height); }
void closeGame() { delete renderer; renderer = nullptr; }
int main(int argc, char** argv) {
    glutInit(&argc, argv); glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(width, height); glutCreateWindow("Moonlit Village");
    SetWindowTextW(WindowFromDC(wglGetCurrentDC()), L"달빛마을 - 저녁 종");
    if (glewInit() != GLEW_OK || !GLEW_VERSION_3_3) { MessageBoxW(nullptr, L"OpenGL 3.3을 지원하는 그래픽 드라이버가 필요합니다.", L"초기화 오류", MB_OK); return 1; }
    renderer = new Renderer(width, height); if (!renderer->IsInitialized()) { delete renderer; return 1; }
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS); glutIgnoreKeyRepeat(1);
    reset(); lastTime = glutGet(GLUT_ELAPSED_TIME) * .001f;
    glutDisplayFunc(render); glutIdleFunc(idle); glutKeyboardFunc(down); glutKeyboardUpFunc(up); glutReshapeFunc(resize); glutCloseFunc(closeGame);
    glutMainLoop(); return 0;
}
