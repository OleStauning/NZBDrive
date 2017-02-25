using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
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
using NZBDrive.Model;

namespace NZBDrive.View
{
    [ProvideToolboxControl("General", true)]
    public partial class ConnectionStatusView : UserControl
    {

        public ObservableCollection<NewsServerConnectionStatus> ConnectionStatus
        {
            get { return (ObservableCollection<NewsServerConnectionStatus>)GetValue(ConnectionStatusProperty); }
            set { SetValue(ConnectionStatusProperty, value); }
        }

        public static readonly DependencyProperty ConnectionStatusProperty =
            DependencyProperty.Register("ConnectionStatus", typeof(ObservableCollection<NewsServerConnectionStatus>),
            typeof(ConnectionStatusView), new PropertyMetadata(null));



        public ConnectionStatusView()
        {
            InitializeComponent();
            this.DataContext = ConnectionStatus;
        }
    }




}
