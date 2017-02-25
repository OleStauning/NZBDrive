using NZBDrive;
using System;
using System.Collections.Generic;
using System.ComponentModel;
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
    public partial class ThrottlingEditBox : UserControl
    {
        private NewsServerThrottling _newsServerThrottling;

        public NewsServerThrottling NewsServerThrottling
        {
            get
            {
                return _newsServerThrottling;
            }
            set 
            {
                if (_newsServerThrottling != null) throw new ArgumentException("NewsServerThrottling already set");
                _newsServerThrottling = value;
                this.DataContext = value;
            }
        }
        
        public ThrottlingEditBox()
        {
            InitializeComponent();
        }

    }
}
