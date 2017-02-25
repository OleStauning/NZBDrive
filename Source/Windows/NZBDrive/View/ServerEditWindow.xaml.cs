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
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace NZBDrive.View
{
    /// <summary>
    /// Interaction logic for ServerEditPage.xaml
    /// </summary>
    public partial class ServerEditWindow : Window
    {
        public ServerEditWindow()
        {
            InitializeComponent();
        }

        private void OKButton_Click(object sender, RoutedEventArgs e)
        {
            this.DialogResult = true;
            this.Close();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            ServerEdit.TextBoxServerName.CaretIndex = ServerEdit.TextBoxServerName.Text.Length;
            Keyboard.Focus(ServerEdit.TextBoxServerName);
        }

    }
}
