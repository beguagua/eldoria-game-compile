using System;
using System.Collections.Generic;
using System.IO;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Graphics.OpenGL;
using OpenTK.Input;

namespace Eldoria
{
    public class Item { public string Name; public int Count; public Item(string n,int c=1){Name=n;Count=c;} }
    public class Enemy { public Vector3 Position; public int Health=30; public bool Alive=true; public Enemy(float x,float z){Position=new Vector3(x,0,z);} }
    public class Quest { public string Name; public string Description; public int Goal; public int Progress; public bool Complete; public Quest(string n,string d,int g){Name=n;Description=d;Goal=g;} }

    public class PlayerState
    {
        public Vector3 Position=Vector3.Zero; public int Level=1; public int XP; public int Health=100; public int MaxHealth=100; public int Attack=10;
        public readonly List<Item> Inventory=new List<Item>();
        public void AddItem(string name,int amount=1){foreach(var i in Inventory)if(i.Name==name){i.Count+=amount;return;}Inventory.Add(new Item(name,amount));}
        public void GainXP(int amount){XP+=amount;while(XP>=Level*100){XP-=Level*100;Level++;MaxHealth+=15;Attack+=3;Health=MaxHealth;Console.WriteLine("LEVEL UP! You reached level "+Level);}}
    }

    public class Chunk
    {
        public const int Size=16; public readonly int X,Z; public readonly int[,] Height=new int[Size,Size];
        public Chunk(int cx,int cz){X=cx;Z=cz;Generate();}
        void Generate(){for(int x=0;x<Size;x++)for(int z=0;z<Size;z++){int wx=X*Size+x,wz=Z*Size+z;double n=Math.Sin(wx*.16)*1.5+Math.Cos(wz*.13)*1.5+Math.Sin((wx+wz)*.06);Height[x,z]=(int)Math.Floor(n*.6);}}
    }

    public class World
    {
        public readonly Dictionary<string,Chunk> Chunks=new Dictionary<string,Chunk>();
        public Chunk GetChunk(int x,int z){string k=x+","+z;Chunk c;if(!Chunks.TryGetValue(k,out c)){c=new Chunk(x,z);Chunks[k]=c;}return c;}
        public void EnsureAround(Vector3 p,int radius=2){int cx=(int)Math.Floor(p.X/Chunk.Size),cz=(int)Math.Floor(p.Z/Chunk.Size);for(int x=-radius;x<=radius;x++)for(int z=-radius;z<=radius;z++)GetChunk(cx+x,cz+z);}
    }

    public class Game:GameWindow
    {
        PlayerState player=new PlayerState(); World world=new World(); List<Enemy> enemies=new List<Enemy>(); List<Quest> quests=new List<Quest>(); float yaw=45f; double autosave;
        public Game():base(960,540,GraphicsMode.Default,"Eldoria - RPG"){ }
        protected override void OnLoad(EventArgs e){base.OnLoad(e);GL.ClearColor(.45f,.70f,.95f,1);GL.Enable(EnableCap.DepthTest);world.EnsureAround(player.Position);enemies.Add(new Enemy(5,3));enemies.Add(new Enemy(-6,4));quests.Add(new Quest("First Hunt","Defeat 2 monsters",2));LoadGame();}
        protected override void OnResize(EventArgs e){base.OnResize(e);GL.Viewport(0,0,Width,Height);Matrix4 p=Matrix4.CreatePerspectiveFieldOfView(MathHelper.DegreesToRadians(60),Width/(float)Height,.1f,300);GL.MatrixMode(MatrixMode.Projection);GL.LoadMatrix(ref p);}
        protected override void OnUpdateFrame(FrameEventArgs e){base.OnUpdateFrame(e);var k=Keyboard.GetState();if(k.IsKeyDown(Key.Escape))Exit();float speed=k.IsKeyDown(Key.ShiftLeft)?6f:3f;Vector3 d=Vector3.Zero;if(k.IsKeyDown(Key.W))d.Z-=1;if(k.IsKeyDown(Key.S))d.Z+=1;if(k.IsKeyDown(Key.A))d.X-=1;if(k.IsKeyDown(Key.D))d.X+=1;if(d.LengthSquared>0){d.Normalize();player.Position+=d*speed*(float)e.Time;world.EnsureAround(player.Position);}
            if(k.IsKeyDown(Key.Space)){foreach(var n in enemies)if(n.Alive&&(n.Position-player.Position).Length<3){n.Health-=player.Attack;if(n.Health<=0){n.Alive=false;player.GainXP(50);player.AddItem("Monster Core");if(quests[0].Progress<quests[0].Goal)quests[0].Progress++;}}}
            if(k.IsKeyDown(Key.F5))SaveGame();autosave+=e.Time;if(autosave>10){SaveGame();autosave=0;}Title="Eldoria | Level "+player.Level+" | HP "+player.Health+" | XP "+player.XP+" | WASD Move | SPACE Attack | F5 Save";}
        protected override void OnRenderFrame(FrameEventArgs e){GL.Clear(ClearBufferMask.ColorBufferBit|ClearBufferMask.DepthBufferBit);Matrix4 view=Matrix4.LookAt(player.Position+new Vector3(7,7,10),player.Position,Vector3.UnitY);GL.MatrixMode(MatrixMode.Modelview);GL.LoadMatrix(ref view);DrawWorld();DrawPlayer();foreach(var n in enemies)if(n.Alive)DrawCube(n.Position.X,.1f,n.Position.Z,1,1.4f,1,.75f,.15f,.15f);SwapBuffers();}
        void DrawWorld(){int pcx=(int)Math.Floor(player.Position.X/Chunk.Size),pcz=(int)Math.Floor(player.Position.Z/Chunk.Size);for(int cx=pcx-2;cx<=pcx+2;cx++)for(int cz=pcz-2;cz<=pcz+2;cz++){Chunk c=world.GetChunk(cx,cz);for(int x=0;x<Chunk.Size;x++)for(int z=0;z<Chunk.Size;z++){int h=c.Height[x,z];float wx=cx*Chunk.Size+x,wz=cz*Chunk.Size+z;DrawCube(wx,h-.5f,wz,1,1,1,.25f,.65f,.28f);}}}
        void DrawPlayer(){DrawCube(player.Position.X,.7f,player.Position.Z,.7f,1.4f,.7f,.2f,.4f,.9f);DrawCube(player.Position.X,1.75f,player.Position.Z,.55f,.55f,.55f,.95f,.72f,.52f);}
        void DrawCube(float x,float y,float z,float sx,float sy,float sz,float r,float g,float b){GL.PushMatrix();GL.Translate(x,y,z);GL.Scale(sx,sy,sz);GL.Color3(r,g,b);GL.Begin(PrimitiveType.Quads);V(-1,-1,1);V(1,-1,1);V(1,1,1);V(-1,1,1);V(1,-1,-1);V(-1,-1,-1);V(-1,1,-1);V(1,1,-1);V(-1,-1,-1);V(-1,-1,1);V(-1,1,1);V(-1,1,-1);V(1,-1,1);V(1,-1,-1);V(1,1,-1);V(1,1,1);V(-1,1,1);V(1,1,1);V(1,1,-1);V(-1,1,-1);V(-1,-1,-1);V(1,-1,-1);V(1,-1,1);V(-1,-1,1);GL.End();GL.PopMatrix();}
        void V(float x,float y,float z){GL.Vertex3(x*.5f,y*.5f,z*.5f);}
        string SavePath{get{return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Personal),".eldoria_save.txt");}}
        void SaveGame(){try{File.WriteAllText(SavePath,player.Position.X+"|"+player.Position.Y+"|"+player.Position.Z+"|"+player.Level+"|"+player.XP+"|"+player.Health); }catch{} }
        void LoadGame(){try{if(!File.Exists(SavePath))return;string[] a=File.ReadAllText(SavePath).Split('|');player.Position=new Vector3(float.Parse(a[0]),float.Parse(a[1]),float.Parse(a[2]));player.Level=int.Parse(a[3]);player.XP=int.Parse(a[4]);player.Health=int.Parse(a[5]);player.MaxHealth=100+(player.Level-1)*15;player.Attack=10+(player.Level-1)*3;}catch{}}
    }
    public static class Program{public static void Main(){using(var g=new Game())g.Run(60);}}
}
