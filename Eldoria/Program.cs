using System;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Graphics.OpenGL;
using OpenTK.Input;

namespace Eldoria
{
    public class Game : GameWindow
    {
        private float angle;
        private float playerX;
        private float playerZ;

        public Game() : base(960, 540, GraphicsMode.Default, "Eldoria") { }

        protected override void OnLoad(EventArgs e)
        {
            base.OnLoad(e);
            GL.ClearColor(0.45f, 0.70f, 0.95f, 1f);
            GL.Enable(EnableCap.DepthTest);
        }

        protected override void OnResize(EventArgs e)
        {
            base.OnResize(e);
            GL.Viewport(0, 0, Width, Height);
            Matrix4 projection = Matrix4.CreatePerspectiveFieldOfView(
                MathHelper.DegreesToRadians(60f), Width / (float)Height, 0.1f, 200f);
            GL.MatrixMode(MatrixMode.Projection);
            GL.LoadMatrix(ref projection);
        }

        protected override void OnUpdateFrame(FrameEventArgs e)
        {
            base.OnUpdateFrame(e);
            var k = Keyboard.GetState();
            if (k.IsKeyDown(Key.Escape)) Exit();
            if (k.IsKeyDown(Key.W)) playerZ -= 3f * (float)e.Time;
            if (k.IsKeyDown(Key.S)) playerZ += 3f * (float)e.Time;
            if (k.IsKeyDown(Key.A)) playerX -= 3f * (float)e.Time;
            if (k.IsKeyDown(Key.D)) playerX += 3f * (float)e.Time;
            angle += 20f * (float)e.Time;
        }

        protected override void OnRenderFrame(FrameEventArgs e)
        {
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);
            Matrix4 view = Matrix4.LookAt(new Vector3(7, 7, 10), new Vector3(0, 0, 0), Vector3.UnitY);
            GL.MatrixMode(MatrixMode.Modelview);
            GL.LoadMatrix(ref view);

            DrawGround();
            DrawPlayer();
            DrawTree(-4, -3);
            DrawTree(4, -2);
            DrawTree(-3, 4);
            DrawCube(0, -0.5f, -4, 3, 1, 1, 0.55f, 0.32f, 0.18f);
            SwapBuffers();
        }

        private void DrawGround()
        {
            for (int x = -8; x <= 8; x++)
                for (int z = -8; z <= 8; z++)
                    DrawCube(x, -1, z, 1, 1, 1, 0.25f, 0.65f, 0.28f);
        }

        private void DrawPlayer()
        {
            DrawCube(playerX, 0, playerZ, 0.8f, 1.8f, 0.8f, 0.25f, 0.45f, 0.95f);
            DrawCube(playerX, 1.25f, playerZ, 0.65f, 0.65f, 0.65f, 0.95f, 0.72f, 0.52f);
        }

        private void DrawTree(float x, float z)
        {
            DrawCube(x, 0, z, 0.6f, 2.2f, 0.6f, 0.45f, 0.25f, 0.10f);
            DrawCube(x, 1.5f, z, 2.2f, 2.0f, 2.2f, 0.12f, 0.55f, 0.18f);
        }

        private void DrawCube(float x, float y, float z, float sx, float sy, float sz, float r, float g, float b)
        {
            GL.PushMatrix();
            GL.Translate(x, y, z);
            GL.Scale(sx, sy, sz);
            GL.Color3(r, g, b);
            GL.Begin(PrimitiveType.Quads);
            Face(-1, 0, 0, 0, 1, 0, 0, 0, 1);
            Face(1, 0, 0, 0, 1, 0, 0, 0, -1);
            Face(0, -1, 0, 1, 0, 0, 0, 0, 1);
            Face(0, 1, 0, 1, 0, 0, 0, 0, -1);
            Face(0, 0, -1, 1, 0, 0, 0, 1, 0);
            Face(0, 0, 1, 1, 0, 0, 0, -1, 0);
            GL.End();
            GL.PopMatrix();
        }

        private void Face(float nx, float ny, float nz, float ax, float ay, float az, float bx, float by, float bz)
        {
            GL.Normal3(nx, ny, nz);
            GL.Vertex3(-ax - bx, -ay - by, -az - bz);
            GL.Vertex3(ax - bx, ay - by, az - bz);
            GL.Vertex3(ax + bx, ay + by, az + bz);
            GL.Vertex3(-ax + bx, -ay + by, -az + bz);
        }
    }

    public static class Program
    {
        public static void Main() { using (var game = new Game()) game.Run(60); }
    }
}
