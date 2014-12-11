using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Net;
using System.Net.Sockets;
using System.Windows.Forms;
using System.ComponentModel;
using System.Data;
using System.Drawing;

 
namespace BasicManipulation
{    
     
    
    class udpOut
    {
        public double ii;
        public double kk;
    }

    class centerCoords
    {
        public double X;
        public double Y;
        
    }

    class MyClass
    {
        int[,] numbers = new int[3, 2] { 
                                         { 1, 2 }, 
                                         { 3, 4 }, 
                                         { 5, 6 } 
                                       };
        public double tanX;
        public double tanY;
        public double tanresult;
        public double angle;
        public int a;
    }

    
      
    public partial class MainWindow : Window
    {   
        
        
        public MainWindow()
        {
            
            InitializeComponent();
        }

        Window1 popup = new Window1();

                    void touchDown (object sender, TouchEventArgs e)
                    {
                       udpOut ll = new udpOut();
            
                        ll.ii = e.GetTouchPoint(this).Position.X;
                        ll.kk = e.GetTouchPoint(this).Position.Y;
                       
                       try
                       {
                           //UDP stuff
                           UdpClient client = new UdpClient("127.0.0.1", 9930);
                             
                           byte[] data2 = BitConverter.GetBytes(ll.ii);
               
                           client.Send(data2, data2.Length);
               
                          //Output stuff
                           Console.Write(ll.ii + Environment.NewLine);

                           UDP.Text = ll.ii.ToString("F");
               
                       }
           
                       catch(SocketException f)
                       {
                           Console.WriteLine(" {0}", f.ErrorCode);
              
                       }

                    }
 
                    void touchMove (object sender, TouchEventArgs ff)
                    {
                        double result;
                        double Y, X, A, B, C, angleA;
                        //double Asin;
                        double angle, tan;
                        double radians;
                        double sinresult;
                        
                        udpOut mm = new udpOut();
                        centerCoords Center = new centerCoords();

                        Center.X = 960;
                        Center.Y = 540;


                        mm.ii = ff.GetTouchPoint(this).Position.X;
                        mm.kk = ff.GetTouchPoint(this).Position.Y;

                        SlidingX.Text = mm.ii.ToString();
                        SlidingY.Text = mm.kk.ToString();

                        Y = (mm.kk - Center.Y);
                        X = (mm.ii - Center.X);

                        result = Y / X;

                        YoverX.Text = result.ToString();

                        A = Math.Atan(result);
                        B = A * (180 / Math.PI);

                                             
                        _atan.Text = B.ToString();

                        
                        
                     }

                    public void open(object sender, RoutedEventArgs e)
                    {
                        

                        popup.Show();
                       

                        
                    }

                      

    }     

    
    
}

/*
 struct SampleStruct
{
   public int x;
   public int y;

   public SampleStruct(int x, int y)
   {
      this.x = x;
      this.y = y;
   }
}

class SampleClass
{
   public string name;
   public int id;

   public SampleClass() {}

   public SampleClass(int id, string name)
   {
      this.id = id;
      this.name = name;
   }
}

class ProgramClass
{
   static void Main()
   {
      // Create objects using default constructors:
      SampleStruct Location1 = new SampleStruct();
      SampleClass Employee1 = new SampleClass();

      // Display values:
      Console.WriteLine("Default values:");
      Console.WriteLine("   Struct members: {0}, {1}",
             Location1.x, Location1.y);
      Console.WriteLine("   Class members: {0}, {1}",
             Employee1.name, Employee1.id);

      // Create objects using parameterized constructors:
      SampleStruct Location2 = new SampleStruct(10, 20);
      SampleClass Employee2 = new SampleClass(1234, "Cristina Potra");

      // Display values:
      Console.WriteLine("Assigned values:");
      Console.WriteLine("   Struct members: {0}, {1}",
             Location2.x, Location2.y);
      Console.WriteLine("   Class members: {0}, {1}",
             Employee2.name, Employee2.id);
   }
}

Output:
Default values:
   Struct members: 0, 0
   Class members: , 0
Assigned values:
   Struct members: 10, 20
   Class members: Cristina Potra, 1234
*/

/* void Window_ManipulationStarting(object sender, ManipulationStartingEventArgs e)
      {
          e.ManipulationContainer = this;
          e.Handled = true;
      }

      void Window_ManipulationDelta(object sender, ManipulationDeltaEventArgs e)
      {

          // Get the Rectangle and its RenderTransform matrix.
          Rectangle rectToMove = e.OriginalSource as Rectangle;
          Matrix rectsMatrix = ((MatrixTransform)rectToMove.RenderTransform).Matrix;

          // Rotate the Rectangle.
          rectsMatrix.RotateAt(e.DeltaManipulation.Rotation,
                               e.ManipulationOrigin.X,
                               e.ManipulationOrigin.Y);

          // Resize the Rectangle.  Keep it square 
          // so use only the X value of Scale.
          rectsMatrix.ScaleAt(e.DeltaManipulation.Scale.X,
                              e.DeltaManipulation.Scale.X,
                              e.ManipulationOrigin.X,
                              e.ManipulationOrigin.Y);

          // Move the Rectangle.
          rectsMatrix.Translate(e.DeltaManipulation.Translation.X,
                                e.DeltaManipulation.Translation.Y);
                       
                                         
          // Apply the changes to the Rectangle.
          rectToMove.RenderTransform = new MatrixTransform(rectsMatrix);

          Rect containingRect =
              new Rect(((FrameworkElement)e.ManipulationContainer).RenderSize);

          Rect shapeBounds =
              rectToMove.RenderTransform.TransformBounds(
                  new Rect(rectToMove.RenderSize));

          // Check if the rectangle is completely in the window.
          // If it is not and intertia is occuring, stop the manipulation.
          if (e.IsInertial && !containingRect.Contains(shapeBounds))
          {
              e.Complete();
          }


          e.Handled = true;

      }

      void Window_InertiaStarting(object sender, ManipulationInertiaStartingEventArgs e)
      {

          // Decrease the velocity of the Rectangle's movement by 
          // 10 inches per second every second.
          // (10 inches * 96 pixels per inch / 1000ms^2)
          e.TranslationBehavior.DesiredDeceleration = 10.0 * 96.0 / (1000.0 * 1000.0);

          // Decrease the velocity of the Rectangle's resizing by 
          // 0.1 inches per second every second.
          // (0.1 inches * 96 pixels per inch / (1000ms^2)
          e.ExpansionBehavior.DesiredDeceleration = 0.1 * 96 / (1000.0 * 1000.0);

          // Decrease the velocity of the Rectangle's rotation rate by 
          // 2 rotations per second every second.
          // (2 * 360 degrees / (1000ms^2)
          e.RotationBehavior.DesiredDeceleration = 720 / (1000.0 * 1000.0);

          e.Handled = true;
      }*/