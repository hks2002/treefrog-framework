FROM daocloud.io/ubuntu

MAINTAINER <56649783@qq.com>

RUN sudo apt-get update
RUN sudo apt-get install -y qt5-default qt5-qmake libqt5sql5-mysql libqt5sql5-psql libqt5sql5-odbc libqt5sql5-sqlite libqt5core5a libqt5qml5 libqt5xml5 qtbase5-dev qtdeclarative5-dev qtbase5-dev-tools gcc g++ make
RUN sudo apt-get install -y libmysqlclient-dev libpq5 libodbc1
RUN wget https://github.com/hks2002/treefrog-framework/archive/Composite-Primary-Keys.tar.gz
RUN tar -xzf Composite-Primary-Keys.tar.gz
RUN rm Composite-Primary-Keys.tar.gz
RUN cd treefrog-framework-Composite-Primary-Keys
RUN ./configure
RUN cd src
RUN make -j4
RUN sudo make install
RUN cd ../tools
RUN make -j4
RUN sudo make install
RUN cd ../
RUN cd ../
RUN rm -rf treefrog-framework-Composite-Primary-Keys
