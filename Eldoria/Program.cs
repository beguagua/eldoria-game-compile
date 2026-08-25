using System;
using System.Collections.Generic;
using System.IO;
using System.Globalization;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Graphics.OpenGL;
using OpenTK.Input;

namespace Eldoria
{
    public sealed class Item { public string Name; public int Count; public Item(string name,int count=1){Name=name;Count=count;} }
    public sealed class Enemy { public Vector3 Position; public int Health=30; public bool Alive=true; public double Cooldown; public Enemy(float x,float z){Position=new Vector3(x,0,z);} }
    public sealed class Quest { public string Name; public int Goal,Progress; public bool Complete; public Quest(string n,int g){Name=n;Goal=g;} }
    public sealed class PlayerState {
        public Vector3 Position=Vector3.Zero; public int Level=1,XP,Health=100,MaxHealth=100,Attack=10;
        public readonly List<Item> Inventory=new List<Item>();
        public void AddItem(string name,int amount=1){foreach(Item i in Inventory)if(i.Name==name){i.Count+=amount;return;}Inventory.Add(new Item(name,amount));}
        public void GainXP(int amount){XP+=amount;while(XP>=Level*100){XP-=Level*100;Level++;MaxHealth+=15;Attack+=3;Health=MaxHealth;}}
        public void TakeDamage(int damage){if(damage>0)Health=Math.Max(0,Health-damage);}
    }
    public sealed class Chunk {
        public const int Size=16; public readonly int X,Z; public readonly int[,] Height=new int[Size,Size];
        public Chunk(int x,int z){X=x;Z=z;for(int a=0;a<Size;a++)for(int b=0;b<Size;b++){int wx=x*Size+a,wz=z*Size+b;double n=Math.Sin(wx*.16)*1.5+Math.Cos(wz*.13)*1.5+Math.Sin((wx+wz)*.06);Height[a,b]=(int)Math.Floor(n*.6);}}
    }
    public sealed class World {
        public readonly Dictionary<string,Chunk> Chunks=new Dictionary<string,Chunk>();
        public Chunk GetChunk(int x,int z){string k=x+","+z;Chunk c;if(!Chunks.TryGetValue(k,out c)){c=new Chunk(x,z);Chunks[k]=c;}return c;}
        public void Ensure(Vector3 p,int r=2){int cx=(int)Math.Floor(p.X/Chunk.Size),cz=(int)Math.Floor(p.Z/Chunk.Size);for(int x=-r;x<=r;x++)for(int z=-r;z<=r;z++)GetChunk(cx+x,cz+z);}
    }
    public sealed class Game:GameWindow {
        readonly PlayerState player=new PlayerState(); readonly World world=new World(); readonly List<Enemy> enemies=new List<Enemy>(); readonly List<Quest> quests=new List<Quest>();
        double saveTimer; bool attackHeld,saveHeld; double attackCooldown;
        public Game():base(960,540,GraphicsMode.Default,"Eldoria - RPG"){}
        protected override void OnLoad(EventArgs e){base.OnLoad(e);GL.ClearColor(.45f,.70f,.95f,1);GL.Enable(EnableCap.DepthTest);LoadGame();world.Ensure(player.Position);enemies.Add(new Enemy(5,3));enemies.Add(new Enemy(-6,4));enemies.Add(new Enemy(8,-5));quests.Add(new Quest("First Hunt",2));}
        protected override void OnResize(EventArgs e){base.OnResize(e);GL.Viewport(0,0,Width,Height);Matrix4 p=Matrix4.CreatePerspectiveFieldOfView(MathHelper.DegreesToRadians(60),Width/(float)Height,.1f,300);GL.MatrixMode(MatrixMode.Projection);GL.LoadMatrix(ref p);}
        protected override void OnUpdateFrame(FrameEventArgs e){base.OnUpdateFrame(e);KeyboardDevice k=Keyboard.GetState();if(k.IsKeyDown(Key.Escape))Exit();float speed=k.IsKeyDown(Key.ShiftLeft)?6f:3f;Vector3 d=Vector3.Zero;if(k.IsKeyDown(Key.W))d.Z--;if(k.IsKeyDown(Key.S))d.Z++;if(k.IsKeyDown(Key.A))d.X--;if(k.IsKeyDown(Key.D))d.X++;if(d.LengthSquared>0){d.Normalize();player.Position+=d*speed*(float)e.Time;world.Ensure(player.Position);}bool a=k.IsKeyDown(Key.Space);if(a&&!attackHeld)Attack();attackHeld=a;bool s=k.IsKeyDown(Key.F5);if(s&&!saveHeld)SaveGame();saveHeld=s;attackCooldown-=e.Time;saveTimer+=e.Time;UpdateEnemies(e.Time);if(saveTimer>=10){SaveGame();saveTimer=0;}if(player.Health<=0)Respawn();Title="Eldoria | Lv "+player.Level+" | HP "+player.Health+"/"+player.MaxHealth+" | XP "+player.XP+" | WASD Move | SPACE Attack | F5 Save";}
        void Attack(){if(attackCooldown>0)return;attackCooldown=.35;foreach(Enemy n in enemies){if(!n.Alive)continue;if((n.Position-player.Position).Length<=2.5f){n.Health-=player.Attack;if(n.Health<=0){n.Alive=false;player.GainXP(50);player.AddItem("Monster Core");if(quests[0].Progress<quests[0].Goal)quests[0].Progress++;if(quests[0].Progress>=quests[0].Goal)quests[0].Complete=true;}}}}
        void UpdateEnemies(double dt){foreach(Enemy n in enemies){if(!n.Alive)continue;Vector3 d=player.Position-n.Position;float distance=d.Length;if(distance>1.7f&&distance<18){d.Y=0;if(d.LengthSquared>0){d.Normalize();n.Position+=d*(float)(dt*1.2);}}n.Cooldown-=dt;if(distance<=1.7f&&n.Cooldown<=0){player.TakeDamage(5);n.Cooldown=1.2;}}}
        void Respawn(){player.Position=Vector3.Zero;player.Health=player.MaxHealth;foreach(Enemy n in enemies)n.Cooldown=1;}
        protected override void OnRenderFrame(FrameEventArgs e){GL.Clear(ClearBufferMask.ColorBufferBit|ClearBufferMask.DepthBufferBit);Matrix4 v=Matrix4.LookAt(player.Position+new Vector3(7,7,10),player.Position,Vector3.UnitY);GL.MatrixMode(MatrixMode.Modelview);GL.LoadMatrix(ref v);DrawWorld();DrawCube(player.Position.X,.7f,player.Position.Z,.7f,1.4f,.7f,.2f,.4f,.9f);DrawCube(player.Position.X,1.75f,player.Position.Z,.55f,.55f,.55f,.95f,.72f,.52f);foreach(Enemy n in enemies)if(n.Alive)DrawCube(n.Position.X,.1f,n.Position.Z,1,1.4f,1,.75f,.15f,.15f);SwapBuffers();}
        void DrawWorld(){int cx=(int)Math.Floor(player.Position.X/Chunk.Size),cz=(int)Math.Floor(player.Position.Z/Chunk.Size);for(int x=cx-2;x<=cx+2;x++)for(int z=cz-2;z<=cz+2;z++){Chunk c=world.GetChunk(x,z);for(int a=0;a<Chunk.Size;a++)for(int b=0;b<Chunk.Size;b++){int h=c.Height[a,b];DrawCube(x*Chunk.Size+a,h-.5f,z*Chunk.Size+b,1,1,1,.25f,.65f,.28f);}}}
        void DrawCube(float x,float y,float z,float sx,float sy,float sz,float r,float g,float b){GL.PushMatrix();GL.Translate(x,y,z);GL.Scale(sx,sy,sz);GL.Color3(r,g,b);GL.Begin(PrimitiveType.Quads);V(-1,-1,1);V(1,-1,1);V(1,1,1);V(-1,1,1);V(1,-1,-1);V(-1,-1,-1);V(-1,1,-1);V(1,1,-1);V(-1,-1,-1);V(-1,-1,1);V(-1,1,1);V(-1,1,-1);V(1,-1,1);V(1,-1,-1);V(1,1,-1);V(1,1,1);V(-1,1,1);V(1,1,1);V(1,1,-1);V(-1,1,-1);V(-1,-1,-1);V(1,-1,-1);V(1,-1,1);V(-1,-1,1);GL.End();GL.PopMatrix();}
        void V(float x,float y,float z){GL.Vertex3(x*.5f,y*.5f,z*.5f);}
        string SavePath{get{return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Personal),".eldoria_save.txt");}}
        void SaveGame(){try{File.WriteAllText(SavePath,string.Join("|",player.Position.X.ToString(CultureInfo.InvariantCulture),player.Position.Y.ToString(CultureInfo.InvariantCulture),player.Position.Z.ToString(CultureInfo.InvariantCulture),player.Level,player.XP,player.Health));}catch{}}
        void LoadGame(){try{if(!File.Exists(SavePath))return;string[] a=File.ReadAllText(SavePath).Split('|');player.Position=new Vector3(float.Parse(a[0],CultureInfo.InvariantCulture),float.Parse(a[1],CultureInfo.InvariantCulture),float.Parse(a[2],CultureInfo.InvariantCulture));player.Level=int.Parse(a[3]);player.XP=int.Parse(a[4]);player.Health=int.Parse(a[5]);player.MaxHealth=100+(player.Level-1)*15;player.Attack=10+(player.Level-1)*3;}catch{}}
    }
    public static class Program{public static void Main(){using(Game g=new Game())g.Run(60);}}
}
