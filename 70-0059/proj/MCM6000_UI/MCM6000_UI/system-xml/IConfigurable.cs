using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SystemXMLParser
{
    /**
     * Interface that configures the controller when the configure function is called.
     */
    public interface IConfigurable
    {
        void Configure(ConfigurationParams args);
    }

    public interface IPersistableConfigurable : IConfigurable
    {
        void Configure(ConfigurationParams args, bool persist);
    }
}
