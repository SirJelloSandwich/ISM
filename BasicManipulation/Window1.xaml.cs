using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;


namespace BasicManipulation
{
    /// <summary>
    /// Interaction logic for Window1.xaml
    /// </summary>

    struct kp
    {
        public int b;
    };
    
    
 
    public partial class Window1 : Window
    {
       
      int tt = new int();
    


        public Window1()
        {
            InitializeComponent();
            
            
        }

        private void Close(object sender, RoutedEventArgs e)
        {
           
            
            Close();
            
        }


        private void run(object sender, RoutedEventArgs e)
        {
            tt++;
            Ctr.Text = tt.ToString();
        }

             
         
    }
}
