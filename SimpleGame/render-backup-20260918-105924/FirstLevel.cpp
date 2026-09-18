#include "stdafx.h"
#include "FirstLevel.h"
#include "ModelCache.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <vector>
#include <utility>
#include <cassert>
#include <iterator>

namespace FirstLevel {
namespace {
constexpr int Size=40;
constexpr int TargetLevel=5;
constexpr int RequiredAllocation=4;
constexpr int MaxLevel=20;
constexpr float BodyRadius=.20f;
struct Point {float x,y;};
float distance(Point a,Point b){return std::hypot(a.x-b.x,a.y-b.y);}
Point iso(Point p){return {(p.x-p.y)*42,-(p.x+p.y)*21};}
enum Tile {Grass,Water,Rock,Tree,Path};
struct Enemy {Point p,home;int hp=32;float cooldown=0,windup=0,respawn=0,flash=0,phase=0;};
enum Item {Potion,Shard,Blade};
struct Drop {Point p;Item kind;};
struct FloatText {Point p;std::wstring text;float life;bool damage;};
struct State {
    ModelCache models;
    std::mt19937 rng{std::random_device{}()};
    unsigned seed=0;
    std::array<Tile,Size*Size> tiles;
    std::array<int,Size*Size> flow;
    std::vector<Enemy> enemies;
    std::vector<Drop> drops;
    std::vector<FloatText> floating;
    Point player{20.5f,20.5f},direction{0,-1};
    Point mouseDirection{0,-1},slashDirection{0,-1};
    int xp=0,totalXp=0,level=1,hp=100,points=0,strength=0,vitality=0,guard=0;
    int potions=3,shards=0,kills=0,pickups=0,weapon=0;
    bool held[256]={},statsOpen=false,inventoryOpen=false,complete=false,dead=false;
    bool mouseAttack=false;
    int reachableTiles=0;
    float time=0,walk=0,attackCooldown=0,slash=0,invulnerable=0,flowTimer=0,noticeTimer=0;
    bool moving=false;
    std::wstring notice;
    int maxHp()const{return 100+(level-1)*20+vitality*12;}
    int attack()const{return 12+(level-1)*3+strength*2+weapon*5;}
    int defense()const{return 2+level-1+guard;}
    int threshold()const{return 40+(level-1)*30;}
};
std::unique_ptr<State> game;
const Point Camp={20.5f,20.5f};
int index(int x,int y){return y*Size+x;}
bool inside(int x,int y){return x>0&&y>0&&x<Size-1&&y<Size-1;}
bool open(const State& s,int x,int y){return inside(x,y)&&(s.tiles[index(x,y)]==Grass||s.tiles[index(x,y)]==Path);}
bool canStand(const State& s,Point p){
    for(float x:{-BodyRadius,BodyRadius})for(float y:{-BodyRadius,BodyRadius})if(!open(s,int(std::floor(p.x+x)),int(std::floor(p.y+y))))return false;
    return true;
}
void move(State& s,Point& p,Point velocity,float dt){
    Point test={p.x+velocity.x*dt,p.y};if(canStand(s,test))p=test;
    test={p.x,p.y+velocity.y*dt};if(canStand(s,test))p=test;
}
bool lineClear(const State& s,Point a,Point b){
    int steps=std::max(1,int(distance(a,b)*12));
    for(int i=0;i<=steps;++i){float t=float(i)/steps;Point p={a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t};
        if(!open(s,int(std::floor(p.x)),int(std::floor(p.y))))return false;}
    return true;
}
void flood(State& s,int source){
    s.flow.fill(-1);std::queue<int> q;q.push(source);s.flow[source]=0;
    while(!q.empty()){int id=q.front();q.pop();int x=id%Size,y=id/Size;
        for(auto d:{std::pair<int,int>{1,0},{-1,0},{0,1},{0,-1}}){int nx=x+d.first,ny=y+d.second;
            if(open(s,nx,ny)&&s.flow[index(nx,ny)]<0){s.flow[index(nx,ny)]=s.flow[id]+1;q.push(index(nx,ny));}}}
}
void say(State& s,const std::wstring& text){s.notice=text;s.noticeTimer=6;}
void checkComplete(State& s){
    if(!s.complete&&s.level>=TargetLevel&&s.strength+s.vitality+s.guard>=RequiredAllocation&&s.pickups>0){s.complete=true;
        say(s,L"첫 레벨 완료\n전투·아이템 획득·레벨업·능력치 배분을 마쳤습니다. 계속 사냥할 수 있습니다.");}
}
void generate(State& s){
    s.seed=s.rng();std::mt19937 random(s.seed);s.tiles.fill(Grass);
    for(int x=0;x<Size;++x)for(int y=0;y<Size;++y)if(!inside(x,y))s.tiles[index(x,y)]=Tree;
    for(int i=0;i<60;++i){int cx=2+random()%36,cy=2+random()%36,rx=1+random()%3,ry=1+random()%3;
        Tile type=Tile(1+random()%3);
        for(int x=cx-rx;x<=cx+rx;++x)for(int y=cy-ry;y<=cy+ry;++y)
            if(inside(x,y)&&std::pow(float(x-cx)/rx,2)+std::pow(float(y-cy)/ry,2)<=1&&distance({x+.5f,y+.5f},Camp)>4)
                s.tiles[index(x,y)]=type;
    }
    // Connect every walkable component to camp. No walkable island is discarded.
    flood(s,index(20,20));
    for(int y=1;y<Size-1;++y)for(int x=1;x<Size-1;++x)if(open(s,x,y)&&s.flow[index(x,y)]<0){
        int cx=x,cy=y;
        while(cx!=20){s.tiles[index(cx,cy)]=Path;cx+=cx<20?1:-1;}
        while(cy!=20){s.tiles[index(cx,cy)]=Path;cy+=cy<20?1:-1;}
        s.tiles[index(20,20)]=Path;flood(s,index(20,20));
    }
    // A second flood validates the same grid used by collision and enemy navigation.
    flood(s,index(20,20));
    bool valid=true;
    for(int y=1;y<Size-1;++y)for(int x=1;x<Size-1;++x)if(open(s,x,y)) {
        Point center={x+.5f,y+.5f};
        if(s.flow[index(x,y)]<0||!canStand(s,center))valid=false;
        // Adjacent cell centers must also admit the character's full footprint.
        for(auto d:{std::pair<int,int>{1,0},{0,1}})if(open(s,x+d.first,y+d.second))
            for(int step=1;step<4;++step)
                if(!canStand(s,{center.x+d.first*step*.25f,center.y+d.second*step*.25f}))valid=false;
    }
    if(!valid){for(int y=1;y<Size-1;++y)for(int x=1;x<Size-1;++x)s.tiles[index(x,y)]=Grass;flood(s,index(20,20));}
    s.reachableTiles=0;
    for(int y=1;y<Size-1;++y)for(int x=1;x<Size-1;++x)if(open(s,x,y)) {
        assert(s.flow[index(x,y)]>=0);
        ++s.reachableTiles;
    }
    std::cout<<"Level 1 seed="<<s.seed<<", connected walkable cells="<<s.reachableTiles<<std::endl;
    std::vector<Point> candidates;
    for(int y=1;y<Size-1;++y)for(int x=1;x<Size-1;++x)if(open(s,x,y)&&distance({x+.5f,y+.5f},Camp)>5)candidates.push_back({x+.5f,y+.5f});
    std::shuffle(candidates.begin(),candidates.end(),random);s.enemies.clear();
    for(Point p:candidates){bool free=true;for(const auto& e:s.enemies)if(distance(e.p,p)<2.5f)free=false;
        if(free){Enemy e;e.p=e.home=p;e.phase=float(random()%100);s.enemies.push_back(e);}if(s.enemies.size()==26)break;}
    s.drops={{{21.5f,20.5f},Potion},{{19.5f,20.5f},Blade}};
}
void restart(State& s){
    s.player=Camp;s.direction={0,-1};s.xp=s.totalXp=0;s.level=1;s.hp=100;s.points=s.strength=s.vitality=s.guard=0;
    s.potions=3;s.shards=s.kills=s.pickups=s.weapon=0;s.complete=s.dead=s.moving=false;s.statsOpen=s.inventoryOpen=false;
    s.attackCooldown=s.slash=s.invulnerable=s.flowTimer=0;s.floating.clear();std::fill(std::begin(s.held),std::end(s.held),false);
    s.mouseAttack=false;s.walk=0;
    generate(s);say(s,L"첫 레벨 · 잔향의 숲\n경험치를 모아 레벨 5에 도달하고 능력 포인트를 4점 이상 배분하세요.");
}
void addXp(State& s,int amount){
    s.totalXp+=amount;
    if(s.level==MaxLevel)return;
    s.xp+=amount;
    while(s.level<MaxLevel&&s.xp>=s.threshold()){
        s.xp-=s.threshold();++s.level;s.points+=2;s.hp=s.maxHp();
        say(s,L"레벨 "+std::to_wstring(s.level)+L" 달성! 최대 체력 +20 · 공격 +3 · 방어 +1\n능력 포인트 2점을 얻었습니다. C를 눌러 배분하세요.");
    }
    if(s.level==MaxLevel)s.xp=0;
    checkComplete(s);
}
void attack(State& s){
    if(s.dead||s.attackCooldown>0||s.statsOpen||s.inventoryOpen)return;
    s.attackCooldown=.48f;s.slash=.20f;
    s.slashDirection=s.mouseAttack?s.mouseDirection:s.direction;
    for(auto& e:s.enemies){if(e.hp<=0)continue;float d=distance(e.p,s.player);
        float dot=(e.p.x-s.player.x)*s.slashDirection.x+(e.p.y-s.player.y)*s.slashDirection.y;
        if(d<1.8f&&(d<.5f||dot/d>.20f)&&lineClear(s,s.player,e.p)){
            int damage=s.attack();e.hp-=damage;e.flash=.16f;
            // A successful strike interrupts the telegraphed counterattack.
            e.windup=0;e.cooldown=std::max(e.cooldown,.3f);
            s.floating.push_back({e.p,std::to_wstring(damage),.8f,true});
            if(e.hp<=0){++s.kills;addXp(s,20);s.drops.push_back({e.p,s.kills%3==0?Potion:Shard});
                s.floating.push_back({e.p,L"경험치 +20",1.5f,false});e.respawn=14;e.windup=0;}
        }
    }
}
void pickup(State& s){
    if(s.dead||s.statsOpen||s.inventoryOpen)return;
    bool picked=false;
    for(auto it=s.drops.begin();it!=s.drops.end();){
        if(distance(s.player,it->p)<1.3f&&lineClear(s,s.player,it->p)){
            ++s.pickups;picked=true;
            if(it->kind==Potion){++s.potions;say(s,L"회복약을 얻었습니다. Q로 사용할 수 있습니다.");}
            else if(it->kind==Blade){s.weapon=1;say(s,L"수련검을 장착했습니다. 공격력 +5");}
            else {++s.shards;say(s,L"기억 조각을 얻었습니다. I에서 소지품을 확인할 수 있습니다.");}
            it=s.drops.erase(it);
        }else ++it;
    }
    if(!picked&&distance(s.player,Camp)<2.8f) {
        s.hp=s.maxHp();say(s,L"모닥불 곁에서 잠시 쉬었습니다. 체력을 모두 회복했습니다.");
    }
    checkComplete(s);
}
void potion(State& s){
    if(s.dead)return;
    if(s.potions==0){say(s,L"회복약이 없습니다.");return;}
    if(s.hp==s.maxHp()){say(s,L"체력이 가득 차 있습니다.");return;}
    --s.potions;s.hp=std::min(s.maxHp(),s.hp+50);say(s,L"회복약을 사용했습니다. 체력 +50");
}
void allocate(State& s,unsigned char key){
    if(!s.statsOpen||s.points<=0||s.dead)return;
    if(key=='1'){++s.strength;}else if(key=='2'){++s.vitality;s.hp+=12;}else if(key=='3'){++s.guard;}else return;
    --s.points;say(s,L"능력치가 즉시 반영되었습니다.");
    checkComplete(s);
}
Point screen(Point p,Point camera){Point v=iso(p);return {v.x-camera.x,v.y-camera.y};}
void rectangle(Renderer& r,float x,float y,float w,float h,float red,float green,float blue,float alpha=1){
    r.DrawSolidQuad(x,y,x+w,y,x+w,y+h,x,y+h,red,green,blue,alpha);
}
void label(Renderer& r,float x,float y,const std::wstring& text,float red=.93f,float green=.94f,float blue=.90f){r.DrawText(x,y,text.c_str(),red,green,blue,1);}
std::wstring wrapped(Renderer& r,const std::wstring& text,float width){
    std::wstring result;float used=0;
    for(auto c:text){if(c==L'\n'){result+=c;used=0;continue;}float w=r.TextAdvance(c);
        if(used+w>width&&used>0){result+=L'\n';used=0;}result+=c;used+=w;}return result;
}
void panel(Renderer& r,float x,float top,float w,const std::wstring& content){
    auto lines=wrapped(r,content,w-32);float h=32*(1+int(std::count(lines.begin(),lines.end(),L'\n')))+28;
    rectangle(r,x,top-h,w,h,.045f,.065f,.075f,.94f);label(r,x+16,top-30,lines);
}
}
void Initialize(){if(game)return;game.reset(new State);game->models.Load();std::wcout<<game->models.Status()<<std::endl;restart(*game);}
void Shutdown(){game.reset();}
void KeyDown(unsigned char key){
    if(!game)return;State& s=*game;if(s.held[key])return;s.held[key]=true;
    if(key=='r'){restart(s);return;}if(key=='c'){s.statsOpen=!s.statsOpen;s.inventoryOpen=false;s.mouseAttack=false;return;}
    if(key=='i'){s.inventoryOpen=!s.inventoryOpen;s.statsOpen=false;s.mouseAttack=false;return;}
    if(key=='e')pickup(s);if(key=='q')potion(s);if(key==' ')attack(s);allocate(s,key);
}
void KeyUp(unsigned char key){if(game)game->held[key]=false;}
void ClearInput(){if(game){std::fill(std::begin(game->held),std::end(game->held),false);game->mouseAttack=false;game->moving=false;}}
void MouseButton(bool pressed,int x,int y,int width,int height){
    if(!game)return;State& s=*game;
    s.mouseAttack=pressed&&!s.dead&&!s.statsOpen&&!s.inventoryOpen;
    if(!s.mouseAttack)return;
    // Invert the same projection used by Draw, relative to the player at screen y=20.
    float sx=x-width*.5f,sy=height*.5f-y-20;
    Point direction={sx/84.f-sy/42.f,-sx/84.f-sy/42.f};
    float length=std::hypot(direction.x,direction.y);
    if(length>.01f)s.mouseDirection={direction.x/length,direction.y/length};
    attack(s);
}
void Update(float dt){
    if(!game)return;State& s=*game;s.time+=dt;
    if(s.statsOpen||s.inventoryOpen){s.moving=false;return;}
    s.noticeTimer-=dt;
    s.attackCooldown=std::max(0.f,s.attackCooldown-dt);s.slash=std::max(0.f,s.slash-dt);s.invulnerable=std::max(0.f,s.invulnerable-dt);
    for(auto& f:s.floating)f.life-=dt;
    s.floating.erase(std::remove_if(s.floating.begin(),s.floating.end(),[](const FloatText& f){return f.life<=0;}),s.floating.end());
    if(s.dead){s.moving=false;return;}
    float x=float(s.held['d'])-s.held['a'],y=float(s.held['w'])-s.held['s'];Point v={x-y,-x-y};float len=std::hypot(v.x,v.y);
    Point old=s.player;
    if(len>0){s.direction={v.x/len,v.y/len};move(s,s.player,{s.direction.x*3.7f,s.direction.y*3.7f},dt);}
    s.moving=distance(old,s.player)>.0001f;if(s.moving)s.walk+=dt*10;
    if(s.held[' ']||s.mouseAttack)attack(s);
    s.flowTimer-=dt;if(s.flowTimer<=0){s.flowTimer=.20f;flood(s,index(int(s.player.x),int(s.player.y)));}
    for(auto& e:s.enemies){
        e.flash=std::max(0.f,e.flash-dt);e.cooldown=std::max(0.f,e.cooldown-dt);
        if(e.hp<=0){e.respawn-=dt;if(e.respawn<=0&&distance(e.home,s.player)>5){e.p=e.home;e.hp=32;e.cooldown=1;}continue;}
        float d=distance(e.p,s.player);
        if(e.windup>0){e.windup-=dt;if(e.windup<=0){e.cooldown=1.3f;
            if(d<1.35f&&distance(s.player,Camp)>2.8f&&lineClear(s,e.p,s.player)&&s.invulnerable<=0){int damage=std::max(1,12-s.defense());s.hp-=damage;s.invulnerable=.55f;
                s.floating.push_back({s.player,L"-"+std::to_wstring(damage),.9f,true});}
        }}else if(d<1.1f&&e.cooldown<=0&&distance(s.player,Camp)>2.8f){e.windup=.50f;}
        else if(d<8&&d>1&&distance(s.player,Camp)>2.8f){
            int cx=int(e.p.x),cy=int(e.p.y),best=s.flow[index(cx,cy)];Point target=s.player;
            if(best>0){target=e.p;for(auto offset:{std::pair<int,int>{1,0},{-1,0},{0,1},{0,-1}}){int nx=cx+offset.first,ny=cy+offset.second;
                if(open(s,nx,ny)){int cost=s.flow[index(nx,ny)];if(cost>=0&&cost<best){best=cost;target={nx+.5f,ny+.5f};}}}}
            // Align with the current cell before turning into a narrow corridor.
            if(int(target.x)!=cx&&std::abs(e.p.y-(cy+.5f))>.08f)target={e.p.x,cy+.5f};
            else if(int(target.y)!=cy&&std::abs(e.p.x-(cx+.5f))>.08f)target={cx+.5f,e.p.y};
            float length=distance(target,e.p);if(length>.01f){float speed=std::min(1.5f,length/std::max(dt,.001f));move(s,e.p,{(target.x-e.p.x)/length*speed,(target.y-e.p.y)/length*speed},dt);e.phase+=dt*7;}
        }
        if(s.hp<=0){s.hp=0;s.dead=true;say(s,L"쓰러졌습니다. R을 눌러 새 지형에서 다시 시작하세요.");break;}
    }
    checkComplete(s);
}
void Draw(Renderer& r,int width,int height){
    if(!game)return;State& s=*game;Point camera=iso(s.player);camera.y-=20;
    auto visible=[&](Point p,float margin=140.f){Point v=screen(p,camera);return std::abs(v.x)<width*.5f+margin&&std::abs(v.y)<height*.5f+margin;};
    r.SetWorld(camera.x,camera.y,s.time);glClearColor(.12f,.20f,.18f,1);r.BeginFrame();
    for(int y=0;y<Size;++y)for(int x=0;x<Size;++x){Point p={x+.5f,y+.5f};if(!visible(p,80))continue;
        Point v=screen(p,camera);Tile t=s.tiles[index(x,y)];r.SetMaterial(t==Water?8:t==Path?5:2);
        r.DrawSolidQuad(v.x,v.y+21.25f,v.x+42.5f,v.y,v.x,v.y-21.25f,v.x-42.5f,v.y,t==Water?.15f:.28f,t==Water?.39f:.43f,t==Water?.48f:.28f,1);
    }
    r.SetMaterial(0);
    struct Object {Point p;int model;float phase,flash;};std::vector<Object> objects;
    for(int y=0;y<Size;++y)for(int x=0;x<Size;++x){Tile t=s.tiles[index(x,y)];Point p={x+.5f,y+.5f};
        if((t==Rock||t==Tree)&&visible(p))objects.push_back({p,t==Tree?16:17,0,0});}
    for(const auto& e:s.enemies)if(e.hp>0&&visible(e.p))objects.push_back({e.p,8+int(e.phase)%8,0,e.flash>0?.7f:0});
    for(const auto& d:s.drops)if(visible(d.p))objects.push_back({d.p,d.kind==Potion?18:d.kind==Shard?20:21,0,0});
    objects.push_back({Camp,19,0,0});objects.push_back({s.player,s.moving?int(s.walk)%8:0,0,s.invulnerable>0?.6f:0});
    for(const auto& o:objects){Point v=screen(o.p,camera);r.DrawShadow(v.x,v.y,o.model==16?26:13,o.model==16?70:30);}
    std::stable_sort(objects.begin(),objects.end(),[](const Object& a,const Object& b){return a.p.x+a.p.y<b.p.x+b.p.y;});
    for(const auto& o:objects){Point v=screen(o.p,camera);s.models.Draw(r,o.model,v.x,v.y,1,o.flash);if(o.model==19)r.DrawFlame(v.x,v.y+5,27);}
    for(const auto& e:s.enemies)if(e.hp>0&&visible(e.p)&&distance(s.player,e.p)<8){Point v=screen(e.p,camera);
        rectangle(r,v.x-19,v.y+73,38,5,.10f,.10f,.12f);rectangle(r,v.x-19,v.y+73,38*e.hp/32.f,5,.79f,.31f,.27f);
        if(e.windup>0){float a=(.5f-e.windup)/.5f;r.DrawSolidEllipse(v.x,v.y,26*a+7,10*a+3,32,1,.25f,.12f,.35f);}}
    if(s.slash>0){Point center=screen(s.player,camera);float angle=std::atan2(s.slashDirection.y,s.slashDirection.x);
        for(int i=0;i<16;++i){float a=angle-1.1f+i*2.2f/16,b=a+2.2f/16;
            Point va=screen({s.player.x+std::cos(a)*1.8f,s.player.y+std::sin(a)*1.8f},camera);
            Point vb=screen({s.player.x+std::cos(b)*1.8f,s.player.y+std::sin(b)*1.8f},camera);
            r.DrawSolidTriangle(center.x,center.y+25,va.x,va.y+25,vb.x,vb.y+25,.85f,.91f,.78f,s.slash*2.5f);}}
    r.EndFrame(s.time);
    for(const auto& f:s.floating)if(visible(f.p)){Point v=screen(f.p,camera);label(r,v.x-20,v.y+82+(1.5f-f.life)*20,f.text,1,f.damage?.66f:.91f,.60f);}
    float left=-width*.5f+24,top=height*.5f-24;
    std::wstring status=L"잔향의 숲  ·  레벨 "+std::to_wstring(s.level)+L"\n체력 "+std::to_wstring(s.hp)+L" / "+std::to_wstring(s.maxHp())+
        L"    공격 "+std::to_wstring(s.attack())+L"    방어 "+std::to_wstring(s.defense())+
        L"\n경험치 "+(s.level==MaxLevel?std::wstring(L"최고 레벨"):std::to_wstring(s.xp)+L" / "+std::to_wstring(s.threshold()))+L"    처치 "+std::to_wstring(s.kills)+
        L"\n회복약 "+std::to_wstring(s.potions)+L"    미사용 포인트 "+std::to_wstring(s.points);
    panel(r,left,top,std::min(530.f,float(width)-48),status);
    std::wstring goal=s.complete?L"첫 레벨 완료 · 자유 사냥":
        L"레벨 "+std::to_wstring(s.level)+L" / "+std::to_wstring(TargetLevel)+
        L"  ·  능력 배분 "+std::to_wstring(s.strength+s.vitality+s.guard)+L" / "+std::to_wstring(RequiredAllocation)+
        L"\n아이템 획득 "+std::to_wstring(s.pickups)+L"회";
    if(width>=1100)panel(r,width*.5f-380,top,356,goal);
    // The minimap shows blocked terrain, reachable ground, loot and enemies.
    float mx=width*.5f-185,my=height*.5f-(width>=1100?325:445),unit=3.7f;
    rectangle(r,mx-8,my-8,Size*unit+16,Size*unit+16,.04f,.06f,.07f,.9f);
    for(int y=0;y<Size;++y)for(int x=0;x<Size;++x){bool free=open(s,x,y);rectangle(r,mx+x*unit,my+(Size-1-y)*unit,unit,unit,free?.34f:.12f,free?.45f:.20f,free?.35f:.24f);}
    for(const auto& e:s.enemies)if(e.hp>0)rectangle(r,mx+e.p.x*unit-1,my+(Size-e.p.y)*unit-1,3,3,.95f,.36f,.26f);
    for(const auto& d:s.drops)rectangle(r,mx+d.p.x*unit-1,my+(Size-d.p.y)*unit-1,3,3,.92f,.80f,.38f);
    rectangle(r,mx+Camp.x*unit-2,my+(Size-Camp.y)*unit-2,5,5,.98f,.72f,.30f);
    rectangle(r,mx+s.player.x*unit-2,my+(Size-s.player.y)*unit-2,5,5,.84f,.94f,1);
    if(width>=1100)label(r,width*.5f-380,my-35,L"지형 "+std::to_wstring(s.seed));
    if(s.statsOpen){std::wstring stats=L"능력치  ·  남은 포인트 "+std::to_wstring(s.points)+
        L"\n1  힘 +1 → 공격력 +2   ("+std::to_wstring(s.strength)+L")"+
        L"\n2  체력 +1 → 최대 체력 +12   ("+std::to_wstring(s.vitality)+L")"+
        L"\n3  수비 +1 → 방어력 +1   ("+std::to_wstring(s.guard)+L")"+
        L"\n배분 합계 "+std::to_wstring(s.strength+s.vitality+s.guard)+L" / 목표 4"+
        L"\n누적 경험치 "+std::to_wstring(s.totalXp)+L"\nC  닫기";
        float w=std::min(570.f,float(width)-48);panel(r,-w/2,170,w,stats);}
    if(s.inventoryOpen){std::wstring inv=L"소지품\n무기: "+std::wstring(s.weapon?L"수련검 (공격 +5)":L"낡은 검")+
        L"\n회복약: "+std::to_wstring(s.potions)+L"  (Q 사용, 체력 +50)"+
        L"\n기억 조각: "+std::to_wstring(s.shards)+L"\nI  닫기";
        float w=std::min(570.f,float(width)-48);panel(r,-w/2,170,w,inv);}
    float bottom=-height*.5f+24;
    if((s.noticeTimer>0||s.dead)&&!s.statsOpen&&!s.inventoryOpen){
        float w=std::min(920.f,float(width)-48);
        auto lines=wrapped(r,s.notice,w-32);
        float h=32*(1+int(std::count(lines.begin(),lines.end(),L'\n')))+28;
        panel(r,-w/2,bottom+78+h,w,lines);
    }
    label(r,left,bottom+42,L"WASD 이동   Space/좌클릭 공격   E 줍기·휴식   Q 회복약");
    label(r,left,bottom+10,L"C 능력치 (1·2·3 배분)   I 소지품   R 처음부터   T 튜토리얼");
    bool nearby=false;for(const auto& drop:s.drops)if(distance(s.player,drop.p)<1.3f)nearby=true;
    if(!s.dead&&!s.statsOpen&&!s.inventoryOpen) {
        if(nearby)label(r,-95,-82,L"E  아이템 줍기",1,.86f,.49f);
        else if(distance(s.player,Camp)<2.8f)label(r,-150,-82,L"E  모닥불에서 휴식",1,.86f,.49f);
    }
}
}
