.. include:: replace.txt

..
	========================================================================================
	Translated for portuguese by the students of the inter-institutional doctorate program of IME-USP/UTFPR-CM.
	
	Traduzido para o portugu�s pelos alunos do programa de doutorado inter institucional do Instituto de Matem�tica e Estat�stica da Universidade de S�o Paulo --- IME-USP em parceria com a Universidade Tecnol�gica Federal do Paran� - Campus Campo Mour�o --- UTFPR-CM:
	
	* Frank Helbert (frank@ime.usp.br);
	* Luiz Arthur Feitosa dos Santos (luizsan@ime.usp.br);
	* Rodrigo Campiolo (campiolo@ime.usp.br).
	========================================================================================

..
	Tracing

Rastreamento
------------

..
	Background

Introdu��o
**********

..
	As mentioned in the Using the Tracing System section, the whole point of running
	an |ns3| simulation is to generate output for study.  You have two basic 
	strategies to work with in |ns3|: using generic pre-defined bulk output 
	mechanisms and parsing their content to extract interesting information; or 
	somehow developing an output mechanism that conveys exactly (and perhaps only) 
	the information wanted.

Como abordado na se��o Usando o Sistema de Rastreamento, o objetivo principal de uma
simula��o no |ns3| � a gera��o de sa�da para estudo. H� duas estrat�gias b�sicas: 
usar mecanismos predefinidos de sa�da e processar o conte�do para extrair informa��es
relevantes; ou desenvolver mecanismos de sa�da que resultam somente ou exatamente na
informa��o pretendida.

..
	Using pre-defined bulk output mechanisms has the advantage of not requiring any
	changes to |ns3|, but it does require programming.  Often, pcap or NS_LOG
	output messages are gathered during simulation runs and separately run through 
	scripts that use grep, sed or awk to parse the messages and reduce and transform
	the data to a manageable form.  Programs must be written to do the 
	transformation, so this does not come for free.  Of course, if the information
	of interest in does not exist in any of the pre-defined output mechanisms,
	this approach fails.

Usar mecanismos predefinidos de sa�da possui a vantagem de n�o necessitar modifica��es 
no |ns3|, mas requer programa��o. Geralmente, as mensagens de sa�da do pcap ou ``NS_LOG``
s�o coletadas durante a execu��o da simula��o e processadas separadamente por c�digos (`scripts`) que usam `grep`, `sed` ou `awk` para reduzir e transformar os dados para uma forma mais simples de gerenciar. H� o custo do desenvolvimento de programas para realizar as transforma��es e em algumas situa��es a informa��o de interesse pode n�o estar contida em nenhuma das sa�das, logo, a abordagem falha.

..
	If you need to add some tidbit of information to the pre-defined bulk mechanisms,
	this can certainly be done; and if you use one of the |ns3| mechanisms, 
	you may get your code added as a contribution.

Se precisarmos adicionar o m�nimo de informa��o para os mecanismos predefinidos de sa�da, isto certamente pode ser feito e se usarmos os mecanismos do |ns3|, podemos
ter nosso c�digo adicionado como uma contribui��o.

..
	|ns3| provides another mechanism, called Tracing, that avoids some of the 
	problems inherent in the bulk output mechanisms.  It has several important 
	advantages.  First, you can reduce the amount of data you have to manage by only
	tracing the events of interest to you (for large simulations, dumping everything
	to disk for post-processing can create I/O bottlenecks).  Second, if you use this
	method, you can control the format of the output directly so you avoid the 
	postprocessing step with sed or awk script.  If you desire, your output can be 
	formatted directly into a form acceptable by gnuplot, for example.  You can add 
	hooks in the core which can then be accessed by other users, but which will 
	produce no information unless explicitly asked to do so.  For these reasons, we 
	believe that the |ns3| tracing system is the best way to get information 
	out of a simulation and is also therefore one of the most important mechanisms
	to understand in |ns3|.

O |ns3| fornece outro mecanismo, chamado Rastreamento (*Tracing*), que evita alguns dos
problemas associados com os mecanismos de sa�da predefinidos. H� v�rias vantagens. Primeiro, redu��o da quantidade de dados para gerenciar (em simula��es grandes, armazenar toda sa�da no disco pode gerar gargalos de Entrada/Sa�da). Segundo, o formato da sa�da pode ser controlado diretamente evitando o p�s-processamento com c�digos `sed` ou `awk`. Se desejar,
a sa�da pode ser processada diretamente para um formato reconhecido pelo `gnuplot`, por exemplo. Podemos adicionar ganchos ("`hooks`") no n�cleo, os quais podem ser acessados por outros usu�rios, mas que n�o produzir�o nenhuma informa��o exceto que sejam explicitamente solicitados a produzir. Por essas raz�es, acreditamos que o sistema de rastreamento do |ns3| � a melhor forma de obter informa��es fora da simula��o, portanto � um dos mais importantes mecanismos para ser compreendido no |ns3|.

..
	Blunt Instruments

M�todos Simples
+++++++++++++++

..
	There are many ways to get information out of a program.  The most 
	straightforward way is to just directly print the information to the standard 
	output, as in,

H� v�rias formas de obter informa��o ap�s a finaliza��o de um programa. A mais direta
� imprimir a informa��o na sa�da padr�o, como no exemplo,

::

  #include <iostream>
  ...
  void
  SomeFunction (void)
  {
    uint32_t x = SOME_INTERESTING_VALUE;
    ...
    std::cout << "The value of x is " << x << std::endl;
    ...
  } 

..
	Nobody is going to prevent you from going deep into the core of |ns3| and
	adding print statements.  This is insanely easy to do and, after all, you have 
	complete control of your own |ns3| branch.  This will probably not turn 
	out to be very satisfactory in the long term, though.

Ningu�m impedir� que editemos o n�cleo do |ns3| e adicionemos c�digos de impress�o. Isto � simples de fazer, al�m disso temos controle e acesso total ao c�digo fonte do |ns3|. Entretanto, pensando no futuro, isto n�o � muito interessante.

..
	As the number of print statements increases in your programs, the task of 
	dealing with the large number of outputs will become more and more complicated.  
	Eventually, you may feel the need to control what information is being printed 
	in some way; perhaps by turning on and off certain categories of prints, or 
	increasing or decreasing the amount of information you want.  If you continue 
	down this path you may discover that you have re-implemented the ``NS_LOG``
	mechanism.  In order to avoid that, one of the first things you might consider
	is using ``NS_LOG`` itself.

Conforme aumentarmos o n�mero de comandos de impress�o em nossos programas, ficar� mais dif�cil tratar a grande quantidade de sa�das. Eventualmente, precisaremos controlar de alguma maneira qual a informa��o ser� impressa; talvez habilitando ou n�o determinadas categorias de sa�das, ou aumentando ou diminuindo a quantidade de informa��o desejada. Se continuarmos com esse processo, descobriremos depois de um tempo que, reimplementamos o mecanismo ``NS_LOG``. Para evitar isso, utilize o pr�prio ``NS_LOG``.

..
	We mentioned above that one way to get information out of |ns3| is to 
	parse existing NS_LOG output for interesting information.  If you discover that 
	some tidbit of information you need is not present in existing log output, you 
	could edit the core of |ns3| and simply add your interesting information
	to the output stream.  Now, this is certainly better than adding your own
	print statements since it follows |ns3| coding conventions and could 
	potentially be useful to other people as a patch to the existing core.

Como abordado anteriormente, uma maneira de obter informa��o de sa�da do |ns3| � 
processar a sa�da do ``NS_LOG``, filtrando as informa��es relevantes. Se a informa��o
n�o est� presente nos registros existentes, pode-se editar o n�cleo do |ns3| e 
adicionar ao fluxo de sa�da a informa��o desejada. Claro, isto � muito melhor
que adicionar comandos de impress�o, desde que seguindo as conven��es de codifica��o
do |ns3|, al�m do que isto poderia ser potencialmente �til a outras pessoas.

..
	Let's pick a random example.  If you wanted to add more logging to the 
	|ns3| TCP socket (``tcp-socket-base.cc``) you could just add a new 
	message down in the implementation.  Notice that in TcpSocketBase::ReceivedAck()
	there is no log message for the no ack case.  You could simply add one, 
	changing the code from:

Vamos analisar um exemplo, adicionando mais informa��es de registro ao `socket` TCP do arquivo ``tcp-socket-base.cc``, para isto vamos acrescentando uma nova mensagem de registro na implementa��o. Observe que em ``TcpSocketBase::ReceivedAck()`` n�o existem mensagem de registro para casos sem o ACK, ent�o vamos adicionar uma da seguinte forma:

::

  /** Processa o mais recente ACK recebido */
  void
  TcpSocketBase::ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
  {
    NS_LOG_FUNCTION (this << tcpHeader);

    // ACK Recebido. Compara o n�mero ACK com o mais alto seqno n�o confirmado
    if (0 == (tcpHeader.GetFlags () & TcpHeader::ACK))
      { // Ignora se n�o h� flag ACK 
      }
    ...

.. 
	to add a new ``NS_LOG_LOGIC`` in the appropriate statement:

para adicionar um novo ``NS_LOG_LOGIC`` na senten�a apropriada:

::

  /** Processa o mais recente ACK recebido */
  void
  TcpSocketBase::ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
  {
    NS_LOG_FUNCTION (this << tcpHeader);

    // ACK Recebido. Compara o n�mero ACK com o mais alto seqno n�o confirmado
    if (0 == (tcpHeader.GetFlags () & TcpHeader::ACK))
      { // Ignora se n�o h� flag ACK 
        NS_LOG_LOGIC ("TcpSocketBase " << this << " sem flag ACK");
      }
    ...

..
	This may seem fairly simple and satisfying at first glance, but something to
	consider is that you will be writing code to add the ``NS_LOG`` statement 
	and you will also have to write code (as in grep, sed or awk scripts) to parse
	the log output in order to isolate your information.  This is because even 
	though you have some control over what is output by the logging system, you 
	only have control down to the log component level.  

Isto pode parecer simples e satisfat�rio a primeira vista, mas lembre-se que n�s escreveremos
c�digo para adicionar ao ``NS_LOG`` e para processar a sa�da com a finalidade de isolar
a informa��o de interesse. Isto porque o controle � limitado ao n�vel do componente de registro.

..
	If you are adding code to an existing module, you will also have to live with the
	output that every other developer has found interesting.  You may find that in 
	order to get the small amount of information you need, you may have to wade 
	through huge amounts of extraneous messages that are of no interest to you.  You
	may be forced to save huge log files to disk and process them down to a few lines
	whenever you want to do anything.

Se cada desenvolvedor adicionar c�digos de sa�da para um m�dulo existente, logo conviveremos com a sa�da que outro desenvolvedor achou interessante. � descobriremos que para obter uma pequena quantidade de informa��o, precisaremos produzir uma volumosa quantidade de mensagens sem nenhuma relev�ncia (devido aos comandos de sa�da de v�rios desenvolvedores). Assim seremos for�ados a gerar arquivos de registros gigantescos no disco e process�-los para obter poucas linhas de nosso interesse.

..
	Since there are no guarantees in |ns3| about the stability of ``NS_LOG``
	output, you may also discover that pieces of log output on which you depend 
	disappear or change between releases.  If you depend on the structure of the 
	output, you may find other messages being added or deleted which may affect your
	parsing code.

Como n�o h� nenhuma garantia no |ns3| sobre a estabilidade da sa�da do ``NS_LOG``, podemos descobrir que partes do registro de sa�da, que depend�amos, desapareceram ou mudaram entre vers�es. Se dependermos da estrutura da sa�da, podemos encontrar outras mensagens sendo adicionadas ou removidas que podem afetar seu c�digo de processamento.

..
	For these reasons, we consider prints to ``std::cout`` and NS_LOG messages 
	to be quick and dirty ways to get more information out of |ns3|.

Por estas raz�es, devemos considerar o uso do ``std::cout`` e as mensagens ``NS_LOG`` como formas r�pidas e por�m sujas de obter informa��o da sa�da no |ns3|.

..
	It is desirable to have a stable facility using stable APIs that allow one to 
	reach into the core system and only get the information required.  It is
	desirable to be able to do this without having to change and recompile the
	core system.  Even better would be a system that notified the user when an item
	of interest changed or an interesting event happened so the user doesn't have 
	to actively poke around in the system looking for things.

Na grande maioria dos casos desejamos ter um mecanismo est�vel, usando APIs que permitam acessar o n�cleo do sistema e obter somente informa��es interessantes. Isto deve ser poss�vel sem que exista a necessidade de alterar e recompilar o n�cleo do sistema. Melhor ainda seria se um sistema notificasse o usu�rio quando um item de interesse fora modificado ou um evento de interesse aconteceu, pois o usu�rio n�o teria que constantemente vasculhar o sistema procurando por coisas.

..
	The |ns3| tracing system is designed to work along those lines and is 
	well-integrated with the Attribute and Config subsystems allowing for relatively
	simple use scenarios.

O sistema de rastreamento do |ns3| � projetado para trabalhar seguindo essas premissas e � 
integrado com os subsistemas de Atributos (*Attribute*) e Configura��o (*Config*) permitindo cen�rios de uso simples.

.. 
	Overview

Vis�o Geral
***********

..
	The ns-3 tracing system is built on the concepts of independent tracing sources
	and tracing sinks; along with a uniform mechanism for connecting sources to sinks.

O sistema de rastreamento do |ns3| � baseado no conceito independente origem do rastreamento e destino do rastreamento. O |ns3| utiliza um mecanismo uniforme para conectar origens a destinos.

..
	Trace sources are entities that can signal events that happen in a simulation and 
	provide access to interesting underlying data.  For example, a trace source could
	indicate when a packet is received by a net device and provide access to the 
	packet contents for interested trace sinks.  A trace source might also indicate 
	when an interesting state change happens in a model.  For example, the congestion
	window of a TCP model is a prime candidate for a trace source.

As origens do rastreamento (*trace source*) s�o entidades que podem assinalar eventos que ocorrem na simula��o e fornecem acesso a dados de baixo n�vel. Por exemplo, uma origem do rastreamento poderia indicar quando um pacote � recebido por um dispositivo de rede e prove acesso ao conte�do do pacote aos interessados no destino do rastreamento. Uma origem do rastreamento pode tamb�m indicar quando uma mudan�a de estado ocorre em um modelo. Por exemplo, a janela de congestionamento do modelo TCP � um forte candidato para uma origem do rastreamento.

..
	Trace sources are not useful by themselves; they must be connected to other pieces
	of code that actually do something useful with the information provided by the source.
	The entities that consume trace information are called trace sinks.  Trace sources 
	are generators of events and trace sinks are consumers.  This explicit division 
	allows for large numbers of trace sources to be scattered around the system in 
	places which model authors believe might be useful.  

A origem do rastreamento n�o s�o �teis sozinhas; elas devem ser conectadas a outras partes de c�digo que fazem algo �til com a informa��o provida pela origem. As entidades que consomem a informa��o de rastreamento s�o chamadas de destino do rastreamento (*trace sinks*). As origens de rastreamento s�o geradores de eventos e destinos de rastreamento s�o consumidores. Esta divis�o expl�cita permite que in�meras origens de rastreamento estejam dispersas no sistema em locais que os autores do modelo acreditam ser �teis.

..
	There can be zero or more consumers of trace events generated by a trace source.  
	One can think of a trace source as a kind of point-to-multipoint information link.  
	Your code looking for trace events from a particular piece of core code could 
	happily coexist with other code doing something entirely different from the same
	information.

Pode haver zero ou mais consumidores de eventos de rastreamento gerados por uma origem do rastreamento. Podemos pensar em uma origem do rastreamento como um tipo de liga��o de informa��o ponto-para-multiponto. Seu c�digo buscaria por eventos de rastreamento de uma parte espec�fica do c�digo do n�cleo e poderia coexistir com outro c�digo que faz algo inteiramente diferente com a mesma informa��o.

..
	Unless a user connects a trace sink to one of these sources, nothing is output.  By
	using the tracing system, both you and other people at the same trace source are 
	getting exactly what they want and only what they want out of the system.  Neither
	of you are impacting any other user by changing what information is output by the 
	system.  If you happen to add a trace source, your work as a good open-source 
	citizen may allow other users to provide new utilities that are perhaps very useful
	overall, without making any changes to the |ns3| core.  

Ao menos que um usu�rio conecte um destino do rastreamento a uma destas origens, nenhuma sa�da � produzida. Usando o sistema de rastreamento, todos conectados em uma mesma origem do rastreamento est�o obtendo a informa��o que desejam do sistema. Um usu�rio n�o afeta os outros alterando a informa��o provida pela origem. Se acontecer de adicionarmos uma origem do rastreamento, seu trabalho como um bom cidad�o utilizador de c�digo livre pode permitir que outros usu�rios forne�am novas utilidades para todos, sem fazer qualquer modifica��o no n�cleo do |ns3|.
	
.. 
	A Simple Low-Level Example

Um Exemplo Simples de Baixo N�vel
+++++++++++++++++++++++++++++++++

..
	Let's take a few minutes and walk through a simple tracing example.  We are going
	to need a little background on Callbacks to understand what is happening in the
	example, so we have to take a small detour right away.

Vamos gastar alguns minutos para entender um exemplo de rastreamento simples. Primeiramente
precisamos compreender o conceito de *callbacks* para entender o que est� acontecendo
no exemplo.

*Callbacks*
~~~~~~~~~~~

..
	The goal of the Callback system in |ns3| is to allow one piece of code to 
	call a function (or method in C++) without any specific inter-module dependency.
	This ultimately means you need some kind of indirection -- you treat the address
	of the called function as a variable.  This variable is called a pointer-to-function
	variable.  The relationship between function and pointer-to-function pointer is 
	really no different that that of object and pointer-to-object.

O objetivo do sistema de *Callback*, no |ns3|, � permitir a uma parte do c�digo invocar
uma fun��o (ou m�todo em C++) sem qualquer depend�ncia entre m�dulos. Isto � utilizado para prover algum tipo de indire��o -- desta forma tratamos o endere�o da chamada de fun��o como uma vari�vel. Esta vari�vel � denominada vari�vel de ponteiro-para-fun��o. O relacionamento entre fun��o e ponteiro-para-fun��o n�o � t�o diferente que de um objeto e ponteiro-para-objeto.

..
	In C the canonical example of a pointer-to-function is a 
	pointer-to-function-returning-integer (PFI).  For a PFI taking one int parameter,
	this could be declared like,

Em C, o exemplo cl�ssico de um ponteiro-para-fun��o � um ponteiro-para-fun��o-retornando-inteiro (PFI). Para um PFI ter um par�metro inteiro, poderia ser declarado como,

::

  int (*pfi)(int arg) = 0;

..
	What you get from this is a variable named simply "pfi" that is initialized
	to the value 0.  If you want to initialize this pointer to something meaningful,
	you have to have a function with a matching signature.  In this case, you could
	provide a function that looks like,

O c�digo descreve uma vari�vel nomeada como "pfi" que � inicializada com o valor 0. Se quisermos inicializar este ponteiro com um valor significante, temos que ter uma fun��o com uma assinatura id�ntica. Neste caso, poder�amos prover uma fun��o como,

::

  int MyFunction (int arg) {}

..
	If you have this target, you can initialize the variable to point to your
	function:

Dessa forma, podemos inicializar a vari�vel apontando para uma fun��o:

::

  pfi = MyFunction;

..
	You can then call MyFunction indirectly using the more suggestive form of
	the call,

Podemos ent�o chamar ``MyFunction`` indiretamente, usando uma forma mais clara da chamada,

::

  int result = (*pfi) (1234);

..
	This is suggestive since it looks like you are dereferencing the function
	pointer just like you would dereference any pointer.  Typically, however,
	people take advantage of the fact that the compiler knows what is going on
	and will just use a shorter form,

� uma forma mais clara, pois � como se estiv�ssemos dereferenciando o ponteiro da fun��o como dereferenciamos qualquer outro ponteiro. Tipicamente, todavia, usa-se uma forma mais curta pois o compilador sabe o que est� fazendo,

::

  int result = pfi (1234);

..
	This looks like you are calling a function named "pfi," but the compiler is
	smart enough to know to call through the variable ``pfi`` indirectly to
	the function ``MyFunction``.

Esta forma � como se estivessemos chamando uma fun��o nomeada "pfi", mas o compilador reconhece que � uma chamada indireta da fun��o ``MyFunction`` por meio da vari�vel ``pfi``.

..
	Conceptually, this is almost exactly how the tracing system will work.
	Basically, a trace source *is* a callback.  When a trace sink expresses
	interest in receiving trace events, it adds a Callback to a list of Callbacks
	internally held by the trace source.  When an interesting event happens, the 
	trace source invokes its ``operator()`` providing zero or more parameters.
	The ``operator()`` eventually wanders down into the system and does something
	remarkably like the indirect call you just saw.  It provides zero or more 
	parameters (the call to "pfi" above passed one parameter to the target function
	``MyFunction``.

Conceitualmente, � quase exatamente como o sistema de rastreamento funciona. Basicamente, uma origem do rastreamento *�* um *callback*. Quando um destino do rastreamento expressa interesse em receber eventos de rastreamento, ela adiciona a *callback* para a lista de *callbacks*  mantida internamente pela origem do rastreamento. Quando um evento de interesse ocorre, a origem do rastreamento invoca seu ``operator()`` provendo zero ou mais par�metros. O ``operator()`` eventualmente percorre o sistema e faz uma chamada indireta com zero ou mais par�metros.
	
..
	The important difference that the tracing system adds is that for each trace
	source there is an internal list of Callbacks.  Instead of just making one 
	indirect call, a trace source may invoke any number of Callbacks.  When a trace
	sink expresses interest in notifications from a trace source, it basically just
	arranges to add its own function to the callback list.

Uma diferen�a importante � que o sistema de rastreamento adiciona para cada origem do rastreamento uma lista interna de *callbacks*. Ao inv�s de apenas fazer uma chamada indireta, uma origem do rastreamento pode invocar qualquer n�mero de *callbacks*. Quando um destino do rastreamento expressa interesse em notifica��es de uma origem, ela adiciona sua pr�pria fun��o para a lista de *callback*.

..
	If you are interested in more details about how this is actually arranged in 
	|ns3|, feel free to peruse the Callback section of the manual.

Estando interessado em mais detalhes sobre como � organizado o sistema de *callback* no |ns3|, leia a se��o *Callback* do manual.

.. 
	Example Code

C�digo de Exemplo
~~~~~~~~~~~~~~~~~

..
	We have provided some code to implement what is really the simplest example
	of tracing that can be assembled.  You can find this code in the tutorial
	directory as ``fourth.cc``.  Let's walk through it.

Analisaremos uma implementa��o simples de um exemplo de rastreamento. Este c�digo est� no diret�rio do tutorial, no arquivo ``fourth.cc``.

::

  /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
  /*
   * This program is free software; you can redistribute it and/or modify
   * it under the terms of the GNU General Public License version 2 as
   * published by the Free Software Foundation;
   *
   * This program is distributed in the hope that it will be useful,
   * but WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU General Public License for more details.
   *
   * You should have received a copy of the GNU General Public License
   * along with this program; if not, write to the Free Software
   * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   */
  
  #include "ns3/object.h"
  #include "ns3/uinteger.h"
  #include "ns3/traced-value.h"
  #include "ns3/trace-source-accessor.h"
  
  #include <iostream>
  
  using namespace ns3;

..
	Most of this code should be quite familiar to you.  As mentioned above, the
	trace system makes heavy use of the Object and Attribute systems, so you will 
	need to include them.  The first two includes above bring in the declarations
	for those systems explicitly.  You could use the core module header, but this
	illustrates how simple this all really is.  

A maior parte deste c�digo deve ser familiar, pois como j� abordado, o sistema de rastreamento faz uso constante dos sistemas Objeto (*Object*) e Atributos (*Attribute*), logo � necess�rio inclu�-los. As duas primeiras inclus�es (*include*) declaram explicitamente estes dois sistemas. Poder�amos usar o cabe�alho (*header*) do m�dulo n�cleo, este exemplo � simples.

..
	The file, ``traced-value.h`` brings in the required declarations for tracing
	of data that obeys value semantics.  In general, value semantics just means that
	you can pass the object around, not an address.  In order to use value semantics
	at all you have to have an object with an associated copy constructor and 
	assignment operator available.  We extend the requirements to talk about the set
	of operators that are pre-defined for plain-old-data (POD) types.  Operator=, 
	operator++, operator---, operator+, operator==, etc.

O arquivo ``traced-value.h`` � uma declara��o obrigat�ria para rastreamento de dados que usam passagem por valor. Na passagem por valor � passada uma c�pia do objeto e n�o um endere�o. Com a finalidade de usar passagem por valor, precisa-se de um objeto com um construtor de c�pia associado e um operador de atribui��o. O conjunto de operadores predefinidos para tipos de dados primitivos (*plain-old-data*) s�o ++, ---, +, ==, etc.

..
	What this all really means is that you will be able to trace changes to a C++
	object made using those operators.

Isto significa que somos capazes de rastrear altera��es em um objeto C++ usando estes operadores.

..
	Since the tracing system is integrated with Attributes, and Attributes work
	with Objects, there must be an |ns3| ``Object`` for the trace source
	to live in.  The next code snippet declares and defines a simple Object we can
	work with.

Como o sistema de rastreamento � integrado com Atributos e este trabalham com Objetos, deve obrigatoriamente existir um ``Object`` |ns3| para cada origem do rastreamento. O pr�ximo c�digo define e declara um Objeto.

::

  class MyObject : public Object
  {
  public:
    static TypeId GetTypeId (void)
    {
      static TypeId tid = TypeId ("MyObject")
        .SetParent (Object::GetTypeId ())
        .AddConstructor<MyObject> ()
        .AddTraceSource ("MyInteger",
                         "An integer value to trace.",
                         MakeTraceSourceAccessor (&MyObject::m_myInt))
        ;
      return tid;
    }
    
    MyObject () {}
    TracedValue<int32_t> m_myInt;
  };

..
	The two important lines of code, above, with respect to tracing are the 
	``.AddTraceSource`` and the ``TracedValue`` declaration of ``m_myInt``.

As duas linhas mais importantes com rela��o ao rastreamento s�o ``.AddTraceSource`` e a declara��o ``TracedValue`` do ``m_myInt``.


..
	The ``.AddTraceSource`` provides the "hooks" used for connecting the trace
	source to the outside world through the config system.  The ``TracedValue`` 
	declaration provides the infrastructure that overloads the operators mentioned 
	above and drives the callback process.

O m�todo ``.AddTraceSource`` prov� a "liga��o" usada para conectar a origem do rastreamento com o mundo externo, por meio do sistema de configura��o. A declara��o ``TracedValue`` prov� a infraestrutura que sobrecarrega os operadores abordados anteriormente e  gerencia o processo de *callback*.

::

  void
  IntTrace (int32_t oldValue, int32_t newValue)
  {
    std::cout << "Traced " << oldValue << " to " << newValue << std::endl;
  }

..
	This is the definition of the trace sink.  It corresponds directly to a callback
	function.  Once it is connected, this function will be called whenever one of the
	overloaded operators of the ``TracedValue`` is executed.

Esta � a defini��o do destino do rastreamento. Isto corresponde diretamente a fun��o de *callback*. Uma vez que est� conectada, esta fun��o ser� chamada sempre que um dos operadores sobrecarregados de ``TracedValue`` � executado.

..
	We have now seen the trace source and the trace sink.  What remains is code to
	connect the source to the sink.

N�s temos a origem e o destino do rastreamento. O restante � o c�digo para conectar a origem ao destino.

::

  int
  main (int argc, char *argv[])
  {
    Ptr<MyObject> myObject = CreateObject<MyObject> ();
    myObject->TraceConnectWithoutContext ("MyInteger", MakeCallback(&IntTrace));
  
    myObject->m_myInt = 1234;
  }

..
	Here we first create the Object in which the trace source lives.

Criamos primeiro o Objeto no qual est� a origem do rastreamento.

..
	The next step, the ``TraceConnectWithoutContext``, forms the connection
	between the trace source and the trace sink.  Notice the ``MakeCallback``
	template function.  This function does the magic required to create the
	underlying |ns3| Callback object and associate it with the function
	``IntTrace``.  TraceConnect makes the association between your provided
	function and the overloaded ``operator()`` in the traced variable referred 
	to by the "MyInteger" Attribute.  After this association is made, the trace
	source will "fire" your provided callback function.

No pr�ximo passo, o ``TraceConnectWithoutContext`` conecta a origem ao destino do rastreamento. Observe que a fun��o ``MakeCallback`` cria o objeto *callback* e associa com a fun��o ``IntTrace``. ``TraceConnectWithoutContext`` faz a associa��o entre a sua fun��o e o ``operator()``, sobrecarregado a vari�vel rastreada referenciada pelo Atributo ``"MyInteger"``. Depois disso, a origem do rastreamento "disparar�" sua fun��o de callback.

..
	The code to make all of this happen is, of course, non-trivial, but the essence
	is that you are arranging for something that looks just like the ``pfi()``
	example above to be called by the trace source.  The declaration of the 
	``TracedValue<int32_t> m_myInt;`` in the Object itself performs the magic 
	needed to provide the overloaded operators (++, ---, etc.) that will use the
	``operator()`` to actually invoke the Callback with the desired parameters.
	The ``.AddTraceSource`` performs the magic to connect the Callback to the 
	Config system, and ``TraceConnectWithoutContext`` performs the magic to
	connect your function to the trace source, which is specified by Attribute
	name.

O c�digo para fazer isto acontecer n�o � trivial, mas a ess�ncia � a mesma que se a origem do rastreamento chamasse a fun��o ``pfi()`` do exemplo anterior. A declara��o ``TracedValue<int32_t> m_myInt;`` no Objeto � respons�vel pela m�gica dos operadores sobrecarregados que usar�o o ``operator()`` para invocar o *callback*  com os par�metros desejados. O m�todo ``.AddTraceSource`` conecta o *callback* ao sistema de configura��o, e ``TraceConnectWithoutContext`` conecta sua fun��o a fonte de rastreamento, a qual � especificada por um nome 
Atributo.

.. 
	Let's ignore the bit about context for now.

Vamos ignorar um pouco o contexto.

.. 
	Finally, the line,

Finalmente a linha,

::

   myObject->m_myInt = 1234;

..
	should be interpreted as an invocation of ``operator=`` on the member 
	variable ``m_myInt`` with the integer ``1234`` passed as a parameter.

deveria ser interpretada como uma invoca��o do operador ``=`` na vari�vel membro ``m_myInt`` com o inteiro ``1234`` passado como par�metro.

..
	It turns out that this operator is defined (by ``TracedValue``) to execute
	a callback that returns void and takes two integer values as parameters --- 
	an old value and a new value for the integer in question.  That is exactly 
	the function signature for the callback function we provided --- ``IntTrace``.

Por sua vez este operador � definido (por ``TracedValue``) para executar um *callback* que retorna ``void`` e possui dois inteiros como par�metros --- um valor antigo e um novo valor para o inteiro em quest�o. Isto � exatamente a assinatura da fun��o para a fun��o de *callback* que n�s fornecemos --- ``IntTrace``.

..
	To summarize, a trace source is, in essence, a variable that holds a list of
	callbacks.  A trace sink is a function used as the target of a callback.  The
	Attribute and object type information systems are used to provide a way to 
	connect trace sources to trace sinks.  The act of "hitting" a trace source
	is executing an operator on the trace source which fires callbacks.  This 
	results in the trace sink callbacks registering interest in the source being 
	called with the parameters provided by the source.

Para resumir, uma origem do rastreamento �, em ess�ncia, uma vari�vel que mant�m uma lista de *callbacks*. Um destino do rastreamento � uma fun��o usada como alvo da *callback*. O Atributo e os sistemas de informa��o de tipo de objeto s�o usados para fornecer uma maneira de conectar origens e destinos do rastreamento. O a��o de "acionar" uma origem do rastreamento � executar um operador na origem, que dispara os *callbacks*. Isto resulta na execu��o das *callbacks* dos destinos do rastreamento registrados na origem com os par�metros providos pela origem.

.. 
	If you now build and run this example,

Se compilarmos e executarmos este exemplo,

::

  ./waf --run fourth

..
	you will see the output from the ``IntTrace`` function execute as soon as the
	trace source is hit:

observaremos que a sa�da da fun��o ``IntTrace`` � processada logo ap�s a execu��o da
origem do rastreamento:

::

  Traced 0 to 1234

..
	When we executed the code, ``myObject->m_myInt = 1234;``, the trace source 
	fired and automatically provided the before and after values to the trace sink.
	The function ``IntTrace`` then printed this to the standard output.  No 
	problem.

Quando executamos o c�digo,  ``myObject->m_myInt = 1234;`` a origem do rastreamento disparou e automaticamente forneceu os valores anteriores e posteriores para o destino do rastreamento. A fun��o ``IntTrace`` ent�o imprimiu na sa�da padr�o, sem maiores problemas.

.. 
	Using the Config Subsystem to Connect to Trace Sources

Usando o Subsistema de Configura��o para Conectar as Origens de Rastreamento
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

..
	The ``TraceConnectWithoutContext`` call shown above in the simple example is
	actually very rarely used in the system.  More typically, the ``Config``
	subsystem is used to allow selecting a trace source in the system using what is
	called a *config path*.  We saw an example of this in the previous section
	where we hooked the "CourseChange" event when we were playing with 
	``third.cc``.

A chamada ``TraceConnectWithoutContext`` apresentada anteriormente � raramente usada no sistema. Geralmente, o subsistema ``Config`` � usado para selecionar uma origem do rastreamento no sistema usando um caminho de configura��o (*config path*). N�s estudamos um exemplo onde ligamos o evento "CourseChange", quando est�vamos brincando com ``third.cc``.

..
	Recall that we defined a trace sink to print course change information from the
	mobility models of our simulation.  It should now be a lot more clear to you 
	what this function is doing.

N�s definimos um destino do rastreamento para imprimir a informa��o de mudan�a de rota dos modelos de mobilidade de nossa simula��o. Agora est� mais claro o que est� fun��o realizava.

::

  void
  CourseChange (std::string context, Ptr<const MobilityModel> model)
  {
    Vector position = model->GetPosition ();
    NS_LOG_UNCOND (context << 
      " x = " << position.x << ", y = " << position.y);
  }

..
	When we connected the "CourseChange" trace source to the above trace sink,
	we used what is called a "Config Path" to specify the source when we
	arranged a connection between the pre-defined trace source and the new trace 
	sink:

Quando conectamos a origem do rastreamento "CourseChange" para o destino do rastreamento anteriormente, usamos o que � chamado de caminho de configura��o ("`Config Path`") para especificar a origem e o novo destino do rastreamento.

::

  std::ostringstream oss;
  oss <<
    "/NodeList/" << wifiStaNodes.Get (nWifi - 1)->GetId () <<
    "/$ns3::MobilityModel/CourseChange";

  Config::Connect (oss.str (), MakeCallback (&CourseChange));

..
	Let's try and make some sense of what is sometimes considered relatively
	mysterious code.  For the purposes of discussion, assume that the node 
	number returned by the ``GetId()`` is "7".  In this case, the path
	above turns out to be,

Para entendermos melhor o c�digo, suponha que o n�mero do n� retornado por ``GetId()`` � "7". Neste caso, o caminho seria,

::

  "/NodeList/7/$ns3::MobilityModel/CourseChange"

..
	The last segment of a config path must be an ``Attribute`` of an 
	``Object``.  In fact, if you had a pointer to the ``Object`` that has the
	"CourseChange" ``Attribute`` handy, you could write this just like we did 
	in the previous example.  You know by now that we typically store pointers to 
	our nodes in a NodeContainer.  In the ``third.cc`` example, the Nodes of
	interest are stored in the ``wifiStaNodes`` NodeContainer.  In fact, while
	putting the path together, we used this container to get a Ptr<Node> which we
	used to call GetId() on.  We could have used this Ptr<Node> directly to call
	a connect method directly:

O �ltimo segmento de um caminho de configura��o deve ser um Atributo de um 
Objeto. Na verdade, se t�nhamos um ponteiro para o Objeto que tem o Atributo
"CourseChange" ``, poder�amos escrever como no exemplo anterior.
N�s j� sabemos que guardamos tipicamente ponteiros para outros n�s em um ``NodeContainer``. No exemplo ``third.cc``, os n�s de rede de interesse est�o armazenados no ``wifiStaNodes`` ``NodeContainer``. De fato enquanto colocamos o caminho junto usamos este cont�iner para obter um ``Ptr<Node>``, usado na chamada ``GetId()``. Poder�amos usar diretamente o ``Ptr<Node>`` para chamar um m�todo de conex�o.

::

  Ptr<Object> theObject = wifiStaNodes.Get (nWifi - 1);
  theObject->TraceConnectWithoutContext ("CourseChange", MakeCallback (&CourseChange));

..
	In the ``third.cc`` example, we actually want an additional "context" to 
	be delivered along with the Callback parameters (which will be explained below) so we 
	could actually use the following equivalent code,

No exemplo ``third.cc``, queremos um "contexto" adicional para ser encaminhado com os par�metros do *callback* (os quais s�o explicados a seguir) ent�o podemos usar o c�digo equivalente,

::

  Ptr<Object> theObject = wifiStaNodes.Get (nWifi - 1);
  theObject->TraceConnect ("CourseChange", MakeCallback (&CourseChange));

..
	It turns out that the internal code for ``Config::ConnectWithoutContext`` and
	``Config::Connect`` actually do find a Ptr<Object> and call the appropriate
	TraceConnect method at the lowest level.

Acontece que o c�digo interno para ``Config::ConnectWithoutContext`` e ``Config::Connect`` permite localizar um Ptr<Object> e chama o m�todo ``TraceConnect``, no n�vel mais baixo.

..
	The ``Config`` functions take a path that represents a chain of ``Object`` 
	pointers.  Each segment of a path corresponds to an Object Attribute.  The last 
	segment is the Attribute of interest, and prior segments must be typed to contain
	or find Objects.  The ``Config`` code parses and "walks" this path until it 
	gets to the final segment of the path.  It then interprets the last segment as
	an ``Attribute`` on the last Object it found while walking the path.  The  
	``Config`` functions then call the appropriate ``TraceConnect`` or 
	``TraceConnectWithoutContext`` method on the final Object.  Let's see what 
	happens in a bit more detail when the above path is walked.

As fun��es ``Config`` aceitam um caminho que representa uma cadeia de ponteiros de Objetos. Cada segmento do caminho corresponde a um Atributo Objeto. O �ltimo segmento � o Atributo de interesse e os seguimentos anteriores devem ser definidos para conter ou encontrar Objetos. O  c�digo ``Config`` processa o caminho at� obter o segmento final. Ent�o, interpreta o �ltimo segmento como um Atributo no �ltimo Objeto ele encontrou no caminho. Ent�o as fun��es ``Config`` chamam o m�todo ``TraceConnect`` ou ``TraceConnectWithoutContext`` adequado no Objeto final.

Vamos analisar com mais detalhes o processo descrito.

..
	The leading "/" character in the path refers to a so-called namespace.  One 
	of the predefined namespaces in the config system is "NodeList" which is a 
	list of all of the nodes in the simulation.  Items in the list are referred to
	by indices into the list, so "/NodeList/7" refers to the eighth node in the
	list of nodes created during the simulation.  This reference is actually a 
	``Ptr<Node>`` and so is a subclass of an ``ns3::Object``.  

O primeiro caractere "/" no caminho faz refer�ncia a um *namespace*. Um dos *namespaces* predefinidos no sistema de configura��o � "NodeList" que � uma lista de todos os n�s na simula��o. Itens na lista s�o referenciados por �ndices , logo "/NodeList/7" refere-se ao oitavo n� na lista de n�s criados durante a simula��o. Esta refer�ncia � um ``Ptr<Node>``, por consequ�ncia � uma subclasse de um ``ns3::Object``.

..
	As described in the Object Model section of the |ns3| manual, we support
	Object Aggregation.  This allows us to form an association between different 
	Objects without any programming.  Each Object in an Aggregation can be reached 
	from the other Objects.  

Como descrito na se��o Modelo de Objeto do manual |ns3|, h� suporte para Agrega��o de Objeto. Isto permite realizar associa��o entre diferentes Objetos sem qualquer programa��o. Cada Objeto em uma Agrega��o pode ser acessado a partir de outros Objetos.

..
	The next path segment being walked begins with the "$" character.  This 
	indicates to the config system that a ``GetObject`` call should be made 
	looking for the type that follows.  It turns out that the MobilityHelper used in 
	``third.cc`` arranges to Aggregate, or associate, a mobility model to each of 
	the wireless Nodes.  When you add the "$" you are asking for another Object that
	has presumably been previously aggregated.  You can think of this as switching
	pointers from the original Ptr<Node> as specified by "/NodeList/7" to its 
	associated mobility model --- which is of type "$ns3::MobilityModel".  If you
	are familiar with ``GetObject``, we have asked the system to do the following:

O pr�ximo segmento no caminho inicia com o car�cter "$". O cifr�o indica ao sistema de configura��o que uma chamada ``GetObject`` deveria ser realizada procurando o tipo especificado em seguida. � diferente do que o ``MobilityHelper`` usou em ``third.cc`` gerenciar a Agrega��o, ou associar, um modelo de mobilidade para cada dos n�s de rede sem fio. Quando adicionamos o "$", significa que estamos pedindo por outro Objeto que tinha sido presumidamente agregado anteriormente. Podemos pensar nisso como ponteiro de comuta��o do ``Ptr<Node>`` original como especificado por "/NodeList/7" para os modelos de mobilidade associados --- quais s�o do tipo "$ns3::MobilityModel". Se estivermos familiarizados com ``GetObject``, solicitamos ao sistema para fazer o 
seguinte:

::

  Ptr<MobilityModel> mobilityModel = node->GetObject<MobilityModel> ()

..
	We are now at the last Object in the path, so we turn our attention to the 
	Attributes of that Object.  The ``MobilityModel`` class defines an Attribute 
	called "CourseChange".  You can see this by looking at the source code in
	``src/mobility/model/mobility-model.cc`` and searching for "CourseChange" in your
	favorite editor.  You should find,

Estamos no �ltimo Objeto do caminho e neste verificamos os Atributos daquele Objeto. A classe ``MobilityModel`` define um Atributo chamado "CourseChange". Observando o c�digo fonte em ``src/mobility/model/mobility-model.cc`` e procurando por "CourseChange", encontramos,

::

  .AddTraceSource ("CourseChange",
                   "The value of the position and/or velocity vector changed",
                   MakeTraceSourceAccessor (&MobilityModel::m_courseChangeTrace))

.. 
	which should look very familiar at this point.  

o qual parece muito familiar neste momento.

..
	If you look for the corresponding declaration of the underlying traced variable 
	in ``mobility-model.h`` you will find

Se procurarmos por declara��es semelhantes das vari�veis rastreadas em ``mobility-model.h``
encontraremos,

::

  TracedCallback<Ptr<const MobilityModel> > m_courseChangeTrace;

..
	The type declaration ``TracedCallback`` identifies ``m_courseChangeTrace``
	as a special list of Callbacks that can be hooked using the Config functions 
	described above.

A declara��o de tipo ``TracedCallback`` identifica ``m_courseChangeTrace`` como um lista especial de *callbacks* que pode ser ligada usando as fun��es de Configura��o descritas anteriormente.

..
	The ``MobilityModel`` class is designed to be a base class providing a common
	interface for all of the specific subclasses.  If you search down to the end of 
	the file, you will see a method defined called ``NotifyCourseChange()``:

A classe ``MobilityModel`` � projetada para ser a classe base provendo uma interface comum para todas as subclasses. No final do arquivo, encontramos um m�todo chamado ``NotifyCourseChange()``:

::

  void
  MobilityModel::NotifyCourseChange (void) const
  {
    m_courseChangeTrace(this);
  }

..
	Derived classes will call into this method whenever they do a course change to
	support tracing.  This method invokes ``operator()`` on the underlying 
	``m_courseChangeTrace``, which will, in turn, invoke all of the registered
	Callbacks, calling all of the trace sinks that have registered interest in the
	trace source by calling a Config function.

Classes derivadas chamar�o este m�todo toda vez que fizerem uma altera��o na rota para 
suportar rastreamento. Este m�todo invoca ``operator()`` em ``m_courseChangeTrace``, 
que invocar� todos os *callbacks* registrados, chamando todos os *trace sinks* que tem 
interesse registrado na origem do rastreamento usando a fun��o de Configura��o.

..
	So, in the ``third.cc`` example we looked at, whenever a course change is 
	made in one of the ``RandomWalk2dMobilityModel`` instances installed, there
	will be a ``NotifyCourseChange()`` call which calls up into the 
	``MobilityModel`` base class.  As seen above, this invokes ``operator()``
	on ``m_courseChangeTrace``, which in turn, calls any registered trace sinks.
	In the example, the only code registering an interest was the code that provided
	the config path.  Therefore, the ``CourseChange`` function that was hooked 
	from Node number seven will be the only Callback called.

No exemplo ``third.cc`` n�s vimos que sempre que uma mudan�a de rota � realizada em uma das inst�ncias ``RandomWalk2dMobilityModel`` instaladas, haver� uma chamada ``NotifyCourseChange()`` da classe base ``MobilityModel``. Como observado, isto invoca ``operator()`` em ``m_courseChangeTrace``, que por sua vez, chama qualquer destino do rastreamento registrados. No exemplo, o �nico c�digo que registrou interesse foi aquele que forneceu o caminho de configura��o. Consequentemente, a fun��o ``CourseChange`` que foi ligado no Node de n�mero sete ser� a �nica *callback* chamada.

..
	The final piece of the puzzle is the "context".  Recall that we saw an output 
	looking something like the following from ``third.cc``:

A pe�a final do quebra-cabe�a � o "contexto". Lembre-se da sa�da de ``third.cc``:

::

  /NodeList/7/$ns3::MobilityModel/CourseChange x = 7.27897, y = 2.22677

..
	The first part of the output is the context.  It is simply the path through
	which the config code located the trace source.  In the case we have been looking at
	there can be any number of trace sources in the system corresponding to any number
	of nodes with mobility models.  There needs to be some way to identify which trace
	source is actually the one that fired the Callback.  An easy way is to request a 
	trace context when you ``Config::Connect``.

A primeira parte da sa�da � o contexto. � simplesmente o caminho pelo qual o c�digo de configura��o localizou a origem do rastreamento. No caso, poder�amos ter qualquer n�mero de origens de rastreamento no sistema correspondendo a qualquer n�mero de n�s com modelos de mobilidade. � necess�rio uma maneira de identificar qual origem do rastreamento disparou o *callback*. Uma forma simples � solicitar um contexto de rastreamento quando � usado o ``Config::Connect``.

.. 
	How to Find and Connect Trace Sources, and Discover Callback Signatures

Como Localizar e Conectar Origens de Rastreamento, e Descobrir Assinaturas de *Callback*
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

..
	The first question that inevitably comes up for new users of the Tracing system is,
	"okay, I know that there must be trace sources in the simulation core, but how do
	I find out what trace sources are available to me"?  
	
	The second question is, "okay, I found a trace source, how do I figure out the
	config path to use when I connect to it"? 

	The third question is, "okay, I found a trace source, how do I figure out what 
	the return type and formal arguments of my callback function need to be"?

	The fourth question is, "okay, I typed that all in and got this incredibly bizarre
	error message, what in the world does it mean"?

As quest�es que inevitavelmente os novos usu�rios do sistema de Rastreamento fazem, s�o:

1. "Eu sei que existem origens do rastreamento no n�cleo da simula��o, mas como 
   eu descubro quais est�o dispon�veis para mim?"
2. "Eu encontrei uma origem do rastreamento, como eu defino o caminho de configura��o para 
   usar quando eu conectar a origem?"
3. "Eu encontrei uma origem do rastreamento, como eu  defino o tipo de retorno e os 
   argumentos formais da minha fun��o de *callback*?"
4. "Eu fiz tudo corretamente e obtive uma mensagem de erro bizarra, o que isso significa?"

.. 
	What Trace Sources are Available?

Quais Origens de Rastreamento s�o Disponibilizadas
++++++++++++++++++++++++++++++++++++++++++++++++++

..
	The answer to this question is found in the |ns3| Doxygen.  If you go to 
	the project web site, 
	`ns-3 project
	<http://www.nsnam.org>`_, you will find a link to "Documentation" in the navigation bar.  If you select this link, you will be
	taken to our documentation page. There 
	is a link to "Latest Release" that will take you to the documentation
	for the latest stable release of |ns3|.
	If you select the "API Documentation" link, you will be
	taken to the |ns3| API documentation page.

A resposta � encontrada no Doxygen do |ns3|. Acesse o s�tio Web do projeto, `ns-3 project <http://www.nsnam.org>`_, em seguida, "Documentation" na barra de navega��o. Logo ap�s, "Latest Release" e "API Documentation".

Acesse o item "Modules" na documenta��o do NS-3. Agora, selecione o item "C++ Constructs Used by All Modules". Ser�o exibidos quatro t�picos extremamente �teis:

* The list of all trace sources
* The list of all attributes
* The list of all global values
* Debugging

..
	The list of interest to us here is "the list of all trace sources".  Go 
	ahead and select that link.  You will see, perhaps not too surprisingly, a
	list of all of the trace sources available in the |ns3| core.

Estamos interessados em "*the list of all trace sources*" - a lista de todas origens do rastreamento. Selecionando este item, � exibido uma lista com todas origens dispon�veis no n�cleo do |ns3|.

..
	As an example, scroll down to ``ns3::MobilityModel``.  You will find
	an entry for

Como exemplo, ``ns3::MobilityModel``, ter� uma entrada para

::

  CourseChange: The value of the position and/or velocity vector changed 

..
	You should recognize this as the trace source we used in the ``third.cc``
	example.  Perusing this list will be helpful.

No caso, esta foi a origem do rastreamento usada no exemplo ``third.cc``, esta lista ser� muito �til.

.. 
	What String do I use to Connect?

Qual String eu uso para Conectar?
+++++++++++++++++++++++++++++++++

..
	The easiest way to do this is to grep around in the |ns3| codebase for someone
	who has already figured it out,  You should always try to copy someone else's
	working code before you start to write your own.  Try something like:

A forma mais simples � procurar na base de c�digo do |ns3| por algu�m que j� fez uso do caminho de configura��o que precisamos para ligar a fonte de rastreamento. Sempre dever�amos primeiro copiar um c�digo que funciona antes de escrever nosso pr�prio c�digo. Tente usar os comandos:

::

  find . -name '*.cc' | xargs grep CourseChange | grep Connect

..
	and you may find your answer along with working code.  For example, in this
	case, ``./ns-3-dev/examples/wireless/mixed-wireless.cc`` has something
	just waiting for you to use:

e poderemos encontrar um c�digo operacional que atenda nossas necessidades. Por exemplo, neste caso, ``./ns-3-dev/examples/wireless/mixed-wireless.cc`` tem algo que podemos usar:

::

  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange", 
    MakeCallback (&CourseChangeCallback));

..
	If you cannot find any examples in the distribution, you can find this out
	from the |ns3| Doxygen.  It will probably be simplest just to walk 
	through the "CourseChanged" example.

Se n�o localizamos nenhum exemplo na distribui��o, podemos tentar o Doxygen do |ns3|. � provavelmente mais simples que percorrer o exemplo "CourseChanged".

..
	Let's assume that you have just found the "CourseChanged" trace source in 
	"The list of all trace sources" and you want to figure out how to connect to
	it.  You know that you are using (again, from the ``third.cc`` example) an
	``ns3::RandomWalk2dMobilityModel``.  So open the "Class List" book in
	the NS-3 documentation tree by clicking its "+" box.  You will now see a
	list of all of the classes in |ns3|.  Scroll down until you see the
	entry for ``ns3::RandomWalk2dMobilityModel`` and follow that link.
	You should now be looking at the "ns3::RandomWalk2dMobilityModel Class 
	Reference".

Suponha que encontramos a origem do rastreamento "CourseChanged" na "The list of all trace sources" e queremos resolver como nos conectar a ela. Voc� sabe que est� usando um ``ns3::RandomWalk2dMobilityModel``. Logo, acesse o item "Class List" na documenta��o do |ns3|. Ser� exibida a lista de todas as classes. Selecione a entrada para ``ns3::RandomWalk2dMobilityModel`` para exibir a documenta��o da classe.

..
	If you now scroll down to the "Member Function Documentation" section, you
	will see documentation for the ``GetTypeId`` function.  You constructed one
	of these in the simple tracing example above:

Acesse a se��o "Member Function Documentation" e obter� a documenta��o para a fun��o ``GetTypeId``. Voc� construiu uma dessas em um exemplo anterior:
	
::

    static TypeId GetTypeId (void)
    {
      static TypeId tid = TypeId ("MyObject")
        .SetParent (Object::GetTypeId ())
        .AddConstructor<MyObject> ()
        .AddTraceSource ("MyInteger",
                         "An integer value to trace.",
                         MakeTraceSourceAccessor (&MyObject::m_myInt))
        ;
      return tid;
    }

..
	As mentioned above, this is the bit of code that connected the Config 
	and Attribute systems to the underlying trace source.  This is also the
	place where you should start looking for information about the way to 
	connect. 

Como abordado, este c�digo conecta os sistemas *Config* e Atributos � origem do rastreamento. � tamb�m o local onde devemos iniciar a busca por informa��o sobre como conectar.

..
	You are looking at the same information for the RandomWalk2dMobilityModel; and
	the information you want is now right there in front of you in the Doxygen:

Voc� est� observando a mesma informa��o para ``RandomWalk2dMobilityModel``; e a informa��o que voc� precisa est� expl�cita no Doxygen:
	
::

  This object is accessible through the following paths with Config::Set 
  		and Config::Connect: 

  /NodeList/[i]/$ns3::MobilityModel/$ns3::RandomWalk2dMobilityModel 

..
	The documentation tells you how to get to the ``RandomWalk2dMobilityModel`` 
	Object.  Compare the string above with the string we actually used in the 
	example code:

A documenta��o apresenta como obter o Objeto ``RandomWalk2dMobilityModel``. Compare o texto anterior com o texto que n�s usamos no c�digo do exemplo:

::

  "/NodeList/7/$ns3::MobilityModel"

..
	The difference is due to the fact that two ``GetObject`` calls are implied 
	in the string found in the documentation.  The first, for ``$ns3::MobilityModel``
	will query the aggregation for the base class.  The second implied 
	``GetObject`` call, for ``$ns3::RandomWalk2dMobilityModel``, is used to "cast"
	the base class to the concrete implementation class.  The documentation shows 
	both of these operations for you.  It turns out that the actual Attribute you are
	going to be looking for is found in the base class as we have seen.

A diferen�a � que h� duas chamadas ``GetObject`` inclusas no texto da documenta��o. A primeira, para ``$ns3::MobilityModel`` solicita a agrega��o para a classe base. A segunda, para ``$ns3::RandomWalk2dMobilityModel`` � usada como *cast* da classe base para a 
implementa��o concreta da classe.  

.. 
	Look further down in the ``GetTypeId`` doxygen.  You will find,

Analisando ainda mais o ``GetTypeId`` no Doxygen, temos

::

  No TraceSources defined for this type.
  TraceSources defined in parent class ns3::MobilityModel:

  CourseChange: The value of the position and/or velocity vector changed 
  Reimplemented from ns3::MobilityModel

..
	This is exactly what you need to know.  The trace source of interest is found in
	``ns3::MobilityModel`` (which you knew anyway).  The interesting thing this
	bit of Doxygen tells you is that you don't need that extra cast in the config
	path above to get to the concrete class, since the trace source is actually in
	the base class.  Therefore the additional ``GetObject`` is not required and
	you simply use the path:

Isto � exatamente o que precisamos saber. A origem do rastreamento de interesse � encontrada em ``ns3::MobilityModel``.  O interessante � que pela documenta��o n�o � necess�rio o *cast* extra para obter a classe concreta, pois a origem do rastreamento est� na classe base. Por consequ�ncia, o ``GetObject`` adicional n�o � necess�rio e podemos usar o caminho:

::

  /NodeList/[i]/$ns3::MobilityModel

.. 
	which perfectly matches the example path:

que � id�ntico ao caminho do exemplo:

::

  /NodeList/7/$ns3::MobilityModel

.. 
	What Return Value and Formal Arguments?

Quais s�o os Argumentos Formais e o Valor de Retorno?
+++++++++++++++++++++++++++++++++++++++++++++++++++++

..
	The easiest way to do this is to grep around in the |ns3| codebase for someone
	who has already figured it out,  You should always try to copy someone else's
	working code.  Try something like:

A forma mais simples � procurar na base de c�digo do |ns3| por um c�digo existente. Voc� sempre deveria primeiro copiar um c�digo que funciona antes de escrever seu pr�prio. Tente usar os comandos:
	
::

  find . -name '*.cc' | xargs grep CourseChange | grep Connect

..
	and you may find your answer along with working code.  For example, in this
	case, ``./ns-3-dev/examples/wireless/mixed-wireless.cc`` has something
	just waiting for you to use.  You will find

e voc� poder� encontrar c�digo operacional. Por exemplo, neste caso, ``./ns-3-dev/examples/wireless/mixed-wireless.cc`` tem c�digo para ser reaproveitado.

::

  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange", 
    MakeCallback (&CourseChangeCallback));

..
	as a result of your grep.  The ``MakeCallback`` should indicate to you that
	there is a callback function there which you can use.  Sure enough, there is:

como resultado, ``MakeCallback`` indicaria que h� uma fun��o *callback* que pode ser usada.
Para refor�ar:

::

  static void
  CourseChangeCallback (std::string path, Ptr<const MobilityModel> model)
  {
    ...
  }

.. 
	Take my Word for It

Acredite em Minha Palavra
~~~~~~~~~~~~~~~~~~~~~~~~~

..
	If there are no examples to work from, this can be, well, challenging to 
	actually figure out from the source code.

Se n�o h� exemplos, pode ser desafiador descobrir por meio da an�lise do c�digo fonte.

..
	Before embarking on a walkthrough of the code, I'll be kind and just tell you
	a simple way to figure this out:  The return value of your callback will always 
	be void.  The formal parameter list for a ``TracedCallback`` can be found 
	from the template parameter list in the declaration.  Recall that for our
	current example, this is in ``mobility-model.h``, where we have previously
	found:

Antes de aventurar-se no c�digo, diremos algo importante: O valor de retorno de sua *callback* sempre ser� *void*. A lista de par�metros formais para uma ``TracedCallback`` pode ser encontrada no lista de par�metro padr�o na declara��o. Recorde do exemplo atual, isto est� em ``mobility-model.h``, onde encontramos:

::

  TracedCallback<Ptr<const MobilityModel> > m_courseChangeTrace;

..
	There is a one-to-one correspondence between the template parameter list in 
	the declaration and the formal arguments of the callback function.  Here,
	there is one template parameter, which is a ``Ptr<const MobilityModel>``.
	This tells you that you need a function that returns void and takes a
	a ``Ptr<const MobilityModel>``.  For example,

N�o h� uma correspond�ncia de um-para-um entre a lista de par�metro padr�o na declara��o e os argumentos formais da fun��o *callback*. Aqui, h� um par�metro padr�o, que � um ``Ptr<const MobilityModel>``. Isto significa que precisamos de uma fun��o que retorna *void* e possui um par�metro ``Ptr<const MobilityModel>``. Por exemplo,

::

  void
  CourseChangeCallback (Ptr<const MobilityModel> model)
  {
    ...
  }

..
	That's all you need if you want to ``Config::ConnectWithoutContext``.  If
	you want a context, you need to ``Config::Connect`` and use a Callback 
	function that takes a string context, then the required argument.

Isto � tudo que precisamos para ``Config::ConnectWithoutContext``. Se voc� quer um contexto, use ``Config::Connect`` e uma fun��o *callback* que possui como um par�metro uma `string` de contexto, seguido pelo argumento.

::

  void
  CourseChangeCallback (std::string path, Ptr<const MobilityModel> model)
  {
    ...
  }

..
	If you want to ensure that your ``CourseChangeCallback`` is only visible
	in your local file, you can add the keyword ``static`` and come up with:

Para garantir que ``CourseChangeCallback`` seja somente vis�vel em seu arquivo, voc� pode adicionar a palavra chave ``static``, como no exemplo:

::

  static void
  CourseChangeCallback (std::string path, Ptr<const MobilityModel> model)
  {
    ...
  }

.. 
	which is exactly what we used in the ``third.cc`` example.

exatamente o que � usado no exemplo ``third.cc``.

..
	The Hard Way

A Forma Complicada
~~~~~~~~~~~~~~~~~~

..
	This section is entirely optional.  It is going to be a bumpy ride, especially
	for those unfamiliar with the details of templates.  However, if you get through
	this, you will have a very good handle on a lot of the |ns3| low level
	idioms.

Esta se��o � opcional. Pode ser bem penosa para aqueles que conhecem poucos detalhes de tipos parametrizados de dados (*templates*). Entretanto, se continuarmos nessa se��o, mergulharemos em detalhes de baixo n�vel do |ns3|.

..
	So, again, let's figure out what signature of callback function is required for
	the "CourseChange" Attribute.  This is going to be painful, but you only need
	to do this once.  After you get through this, you will be able to just look at
	a ``TracedCallback`` and understand it.

Vamos novamente descobrir qual assinatura da fun��o de *callback* � necess�ria para o Atributo "CourseChange". Isto pode ser doloroso, mas precisamos faz�-lo apenas uma vez. Depois de tudo, voc� ser� capaz de entender um ``TracedCallback``.

..
	The first thing we need to look at is the declaration of the trace source.
	Recall that this is in ``mobility-model.h``, where we have previously
	found:

Primeiro, verificamos a declara��o da origem do rastreamento. Recorde que isto est� em ``mobility-model.h``:
	
::

  TracedCallback<Ptr<const MobilityModel> > m_courseChangeTrace;

..
	This declaration is for a template.  The template parameter is inside the
	angle-brackets, so we are really interested in finding out what that
	``TracedCallback<>`` is.  If you have absolutely no idea where this might
	be found, grep is your friend.	

Esta declara��o � para um *template*. O par�metro do *template* est� entre ``<>``, logo estamos interessados em descobrir o que � ``TracedCallback<>``. Se n�o tem nenhuma ideia de onde pode ser encontrado, use o utilit�rio *grep*.

..
	We are probably going to be interested in some kind of declaration in the 
	|ns3| source, so first change into the ``src`` directory.  Then, 
	we know this declaration is going to have to be in some kind of header file,
	so just grep for it using:

Estamos interessados em uma declara��o similar no c�digo fonte do |ns3|, logo buscamos no diret�rio ``src``. Ent�o, sabemos que esta declara��o tem um arquivo de cabe�alho, e procuramos por ele usando:

::

  find . -name '*.h' | xargs grep TracedCallback

..
	You'll see 124 lines fly by (I piped this through wc to see how bad it was).
	Although that may seem like it, that's not really a lot.  Just pipe the output
	through more and start scanning through it.  On the first page, you will see
	some very suspiciously template-looking stuff.

Obteremos 124 linhas, com este comando. Analisando a sa�da, encontramos alguns *templates* que podem ser �teis.

::

  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::TracedCallback ()
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::ConnectWithoutContext (c ...
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::Connect (const CallbackB ...
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::DisconnectWithoutContext ...
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::Disconnect (const Callba ...
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::operator() (void) const ...
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::operator() (T1 a1) const ...
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::operator() (T1 a1, T2 a2 ...
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::operator() (T1 a1, T2 a2 ...
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::operator() (T1 a1, T2 a2 ...
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::operator() (T1 a1, T2 a2 ...
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::operator() (T1 a1, T2 a2 ...
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::operator() (T1 a1, T2 a2 ...

..
	It turns out that all of this comes from the header file 
	``traced-callback.h`` which sounds very promising.  You can then take a
	look at ``mobility-model.h`` and see that there is a line which confirms
	this hunch:

Observamos que todas linhas s�o do arquivo de cabe�alho ``traced-callback.h``, logo ele parece muito promissor. Para confirmar, verifique o arquivo ``mobility-model.h``  e procure uma linha que corrobore esta suspeita.

::

  #include "ns3/traced-callback.h"

..
	Of course, you could have gone at this from the other direction and started
	by looking at the includes in ``mobility-model.h`` and noticing the 
	include of ``traced-callback.h`` and inferring that this must be the file
	you want.

Observando as inclus�es em ``mobility-model.h``, verifica-se a inclus�o do ``traced-callback.h`` e conclui-se que este deve ser o arquivo.

..
	In either case, the next step is to take a look at ``src/core/model/traced-callback.h``
	in your favorite editor to see what is happening.

O pr�ximo passo � analisar o arquivo ``src/core/model/traced-callback.h`` e entender sua funcionalidade.

.. 
	You will see a comment at the top of the file that should be comforting:

H� um coment�rio no topo do arquivo que deveria ser animador:

::

  An ns3::TracedCallback has almost exactly the same API as a normal ns3::Callback but
  instead of forwarding calls to a single function (as an ns3::Callback normally does),
  it forwards calls to a chain of ns3::Callback.

.. 
	This should sound very familiar and let you know you are on the right track.

Isto deveria ser familiar e confirma que estamos no caminho correto.

.. 
	Just after this comment, you will find,

Depois deste coment�rio, encontraremos

::

  template<typename T1 = empty, typename T2 = empty, 
           typename T3 = empty, typename T4 = empty,
           typename T5 = empty, typename T6 = empty,
           typename T7 = empty, typename T8 = empty>
  class TracedCallback 
  {
    ...

..
	This tells you that TracedCallback is a templated class.  It has eight possible
	type parameters with default values.  Go back and compare this with the 
	declaration you are trying to understand:

Isto significa que TracedCallback � uma classe gen�rica (*templated class*). Possui oito poss�veis tipos de par�metros com valores padr�es. Retorne e compare com a declara��o que voc� est� tentando entender:

::

  TracedCallback<Ptr<const MobilityModel> > m_courseChangeTrace;

..
	The ``typename T1`` in the templated class declaration corresponds to the 
	``Ptr<const MobilityModel>`` in the declaration above.  All of the other
	type parameters are left as defaults.  Looking at the constructor really
	doesn't tell you much.  The one place where you have seen a connection made
	between your Callback function and the tracing system is in the ``Connect``
	and ``ConnectWithoutContext`` functions.  If you scroll down, you will see
	a ``ConnectWithoutContext`` method here:

O ``typename T1`` na declara��o da classe corresponde a ``Ptr<const MobilityModel>`` da declara��o anterior. Todos os outros par�metros s�o padr�es. Observe que o construtor n�o contribui com muita informa��o. O �nico lugar onde h� uma conex�o entre a fun��o *callback* e o sistema de rastreamento � nas fun��es ``Connect`` e ``ConnectWithoutContext``. Como mostrado a seguir:
	
::

  template<typename T1, typename T2, 
           typename T3, typename T4,
           typename T5, typename T6,
           typename T7, typename T8>
  void 
  TracedCallback<T1,T2,T3,T4,T5,T6,T7,T8>::ConnectWithoutContext ...
  {
    Callback<void,T1,T2,T3,T4,T5,T6,T7,T8> cb;
    cb.Assign (callback);
    m_callbackList.push_back (cb);
  }

..
	You are now in the belly of the beast.  When the template is instantiated for
	the declaration above, the compiler will replace ``T1`` with 
	``Ptr<const MobilityModel>``.  

Voc� est� no olho do fura��o. Quando o *template* � instanciado pela declara��o anterior, o compilador substitui ``T1`` por ``Ptr<const MobilityModel>``.

::

  void 
  TracedCallback<Ptr<const MobilityModel>::ConnectWithoutContext ... cb
  {
    Callback<void, Ptr<const MobilityModel> > cb;
    cb.Assign (callback);
    m_callbackList.push_back (cb);
  }

..
	You can now see the implementation of everything we've been talking about.  The
	code creates a Callback of the right type and assigns your function to it.  This
	is the equivalent of the ``pfi = MyFunction`` we discussed at the start of
	this section.  The code then adds the Callback to the list of Callbacks for 
	this source.  The only thing left is to look at the definition of Callback.
	Using the same grep trick as we used to find ``TracedCallback``, you will be
	able to find that the file ``./core/callback.h`` is the one we need to look at.

Podemos observar a implementa��o de tudo que foi explicado at� este ponto. O c�digo cria uma *callback* do tipo adequado e atribui sua fun��o para ela. Isto � equivalente a ``pfi = MyFunction`` discutida anteriormente. O c�digo ent�o adiciona a *callback* para a lista de *callbacks* para esta origem. O que n�o observamos ainda � a defini��o da *callback*. Usando o utilit�rio *grep* podemos encontrar o arquivo ``./core/callback.h`` e verificar a defini��o.

..
	If you look down through the file, you will see a lot of probably almost
	incomprehensible template code.  You will eventually come to some Doxygen for
	the Callback template class, though.  Fortunately, there is some English:

No arquivo h� muito c�digo incompreens�vel. Felizmente h� algum em Ingl�s. 

::

  This class template implements the Functor Design Pattern.
  It is used to declare the type of a Callback:
   - the first non-optional template argument represents
     the return type of the callback.
   - the second optional template argument represents
     the type of the first argument to the callback.
   - the third optional template argument represents
     the type of the second argument to the callback.
   - the fourth optional template argument represents
     the type of the third argument to the callback.
   - the fifth optional template argument represents
     the type of the fourth argument to the callback.
   - the sixth optional template argument represents
     the type of the fifth argument to the callback.

.. 
	We are trying to figure out what the

N�s estamos tentando descobrir o que significa a declara��o 

::

    Callback<void, Ptr<const MobilityModel> > cb;

..
	declaration means.  Now we are in a position to understand that the first
	(non-optional) parameter, ``void``, represents the return type of the 
	Callback.  The second (non-optional) parameter, ``Ptr<const MobilityModel>``
	represents the first argument to the callback.

Agora entendemos que o primeiro par�metro, ``void``, indica o tipo de retorno da *callback*. O segundo par�metro, ``Ptr<const MobilityModel>`` representa o primeiro argumento da *callback*.

..
	The Callback in question is your function to receive the trace events.  From
	this you can infer that you need a function that returns ``void`` and takes
	a ``Ptr<const MobilityModel>``.  For example,

A *callback* em quest�o � a sua fun��o que recebe os eventos de rastreamento. Logo, podemos deduzir que precisamos de uma fun��o que retorna ``void`` e possui um par�metro ``Ptr<const MobilityModel>``. Por exemplo,

::

  void
  CourseChangeCallback (Ptr<const MobilityModel> model)
  {
    ...
  }

..
	That's all you need if you want to ``Config::ConnectWithoutContext``.  If
	you want a context, you need to ``Config::Connect`` and use a Callback 
	function that takes a string context.  This is because the ``Connect``
	function will provide the context for you.  You'll need:

Isto � tudo que precisamos no ``Config::ConnectWithoutContext``. Se voc� quer um contexto, use ``Config::Connect`` e uma fun��o *callback* que possui como um par�metro uma `string` de contexto, seguido pelo argumento.

::

  void
  CourseChangeCallback (std::string path, Ptr<const MobilityModel> model)
  {
    ...
  }

..
	If you want to ensure that your ``CourseChangeCallback`` is only visible
	in your local file, you can add the keyword ``static`` and come up with:

Se queremos garantir que ``CourseChangeCallback`` � vis�vel somente 
em seu arquivo, voc� pode adicionar a palavra chave ``static``, como no exemplo:

::

  static void
  CourseChangeCallback (std::string path, Ptr<const MobilityModel> model)
  {
    ...
  }

..
	which is exactly what we used in the ``third.cc`` example.  Perhaps you
	should now go back and reread the previous section (Take My Word for It).

o que � exatamente usado no exemplo ``third.cc``. Talvez seja interessante reler a se��o (Acredite em Minha Palavra).

..
	If you are interested in more details regarding the implementation of 
	Callbacks, feel free to take a look at the |ns3| manual.  They are one
	of the most frequently used constructs in the low-level parts of |ns3|.
	It is, in my opinion, a quite elegant thing.

H� mais detalhes sobre a implementa��o de *callbacks* no manual do |ns3|. Elas est�o entre os mais usados construtores das partes de baixo-n�vel do |ns3|. Em minha opini�o, algo bastante elegante.

.. 
	What About TracedValue?

E quanto a TracedValue?
+++++++++++++++++++++++

..
	Earlier in this section, we presented a simple piece of code that used a
	``TracedValue<int32_t>`` to demonstrate the basics of the tracing code.
	We just glossed over the way to find the return type and formal arguments
	for the ``TracedValue``.  Rather than go through the whole exercise, we
	will just point you at the correct file, ``src/core/model/traced-value.h`` and
	to the important piece of code:

No in�cio desta se��o, n�s apresentamos uma parte de c�digo simples que usou um ``TracedValue<int32_t>`` para demonstrar o b�sico sobre c�digo de rastreamento. N�s desprezamos os m�todos para encontrar o tipo de retorno e os argumentos formais para o ``TracedValue``. Acelerando o processo, indicamos o arquivo ``src/core/model/traced-value.h`` e a parte relevante do c�digo:

::

  template <typename T>
  class TracedValue
  {
  public:
    ...
    void Set (const T &v) {
      if (m_v != v)
        {
  	m_cb (m_v, v);
  	m_v = v;
        }
    }
    ...
  private:
    T m_v;
    TracedCallback<T,T> m_cb;
  };

..
	Here you see that the ``TracedValue`` is templated, of course.  In the simple
	example case at the start of the section, the typename is int32_t.  This means 
	that the member variable being traced (``m_v`` in the private section of the 
	class) will be an ``int32_t m_v``.  The ``Set`` method will take a 
	``const int32_t &v`` as a parameter.  You should now be able to understand 
	that the ``Set`` code will fire the ``m_cb`` callback with two parameters:
	the first being the current value of the ``TracedValue``; and the second 
	being the new value being set.

Verificamos que ``TracedValue`` � uma classe parametrizada. No caso simples do in�cio da se��o, o nome do tipo � int32_t. Isto significa que  a vari�vel membro sendo rastreada (``m_v`` na se��o privada da classe) ser� ``int32_t m_v``. O m�todo ``Set`` possui um argumento ``const int32_t &v``. Voc� deveria ser capaz de entender que o c�digo ``Set`` dispar� o *callback* ``m_cb`` com dois par�metros: o primeiro sendo o valor atual do ``TracedValue``; e o segundo sendo o novo valor.

..
	The callback, ``m_cb`` is declared as a ``TracedCallback<T, T>`` which
	will correspond to a ``TracedCallback<int32_t, int32_t>`` when the class is 
	instantiated.

A *callback* ``m_cb`` � declarada como um ``TracedCallback<T, T>`` que corresponder� a um ``TracedCallback<int32_t, int32_t>`` quando a classe � instanciada.

..
	Recall that the callback target of a TracedCallback always returns ``void``.  
	Further recall that there is a one-to-one correspondence between the template 
	parameter list in the declaration and the formal arguments of the callback 
	function.  Therefore the callback will need to have a function signature that 
	looks like:

Lembre-se que o destino da *callback* de um TracedCallback sempre retorna ``void``. Lembre tamb�m que h� uma correspond�ncia de um-para-um entre a lista de par�metros polim�rfica e os argumentos formais da fun��o *callback*. Logo, a *callback* precisa ter uma assinatura de fun��o similar a:

::

  void
  MyCallback (int32_t oldValue, int32_t newValue)
  {
    ...
  }

..
	It probably won't surprise you that this is exactly what we provided in that 
	simple example we covered so long ago:

Isto � exatamente o que n�s apresentamos no exemplo simples abordado anteriormente.

::

  void
  IntTrace (int32_t oldValue, int32_t newValue)
  {
    std::cout << "Traced " << oldValue << " to " << newValue << std::endl;
  }

.. 
	A Real Example

Um Exemplo Real
***************

..
	Let's do an example taken from one of the best-known books on TCP around.  
	"TCP/IP Illustrated, Volume 1: The Protocols," by W. Richard Stevens is a 
	classic.  I just flipped the book open and ran across a nice plot of both the 
	congestion window and sequence numbers versus time on page 366.  Stevens calls 
	this, "Figure 21.10. Value of cwnd and send sequence number while data is being 
	transmitted."  Let's just recreate the cwnd part of that plot in |ns3|
	using the tracing system and ``gnuplot``.

Vamos fazer um exemplo retirado do livro "TCP/IP Illustrated, Volume 1: The Protocols" escrito por W. Richard Stevens. Localizei na p�gina 366 do livro um gr�fico da janela de congestionamento e n�meros de sequ�ncia versus tempo. Stevens denomina de "Figure 21.10. Value of cwnd and send sequence number while data is being transmitted." Vamos recriar a parte *cwnd* daquele gr�fico em |ns3| usando o sistema de rastreamento e ``gnuplot``.

.. 
	Are There Trace Sources Available?

H� Fontes de Rastreamento Disponibilizadas?
+++++++++++++++++++++++++++++++++++++++++++

..
	The first thing to think about is how we want to get the data out.  What is it
	that we need to trace?  The first thing to do is to consult "The list of all
	trace sources" to see what we have to work with.  Recall that this is found
	in the |ns3| Doxygen in the "C++ Constructs Used by All Modules" Module section.  If you scroll
	through the list, you will eventually find:

Primeiro devemos pensar sobre como queremos obter os dados de sa�da. O que � que  n�s precisamos rastrear? Consultamos ent�o *"The list of all trace sources"* para sabermos o que temos para trabalhar. Essa se��o encontra-se na documenta��o na se��o *"Module"*, no item *"C++ Constructs Used by All Modules"*. Procurando na lista, encontraremos:

::

  ns3::TcpNewReno
  CongestionWindow: The TCP connection's congestion window

..
	It turns out that the |ns3| TCP implementation lives (mostly) in the 
	file ``src/internet/model/tcp-socket-base.cc`` while congestion control
	variants are in files such as ``src/internet/model/tcp-newreno.cc``.  
	If you don't know this a priori, you can use the recursive grep trick:

A maior parte da implementa��o do TCP no |ns3| est� no arquivo ``src/internet/model/tcp-socket-base.cc`` enquanto variantes do controle de congestionamento est�o em arquivos como ``src/internet/model/tcp-newreno.cc``. Se n�o sabe a priori dessa informa��o, use:

::

  find . -name '*.cc' | xargs grep -i tcp

.. 
	You will find page after page of instances of tcp pointing you to that file. 

Haver� p�ginas de respostas apontando para aquele arquivo.

..
	If you open ``src/internet/model/tcp-newreno.cc`` in your favorite 
	editor, you will see right up at the top of the file, the following declarations:

No in�cio do arquivo ``src/internet/model/tcp-newreno.cc`` h� as seguintes declara��es:

::

  TypeId
  TcpNewReno::GetTypeId ()
  {
    static TypeId tid = TypeId("ns3::TcpNewReno")
      .SetParent<TcpSocketBase> ()
      .AddConstructor<TcpNewReno> ()
      .AddTraceSource ("CongestionWindow",
                       "The TCP connection's congestion window",
                       MakeTraceSourceAccessor (&TcpNewReno::m_cWnd))
      ;
    return tid;
  }

..
	This should tell you to look for the declaration of ``m_cWnd`` in the header
	file ``src/internet/model/tcp-newreno.h``.  If you open this file in your
	favorite editor, you will find:

Isto deveria gui�-lo para localizar a declara��o de ``m_cWnd`` no arquivo de cabe�alho ``src/internet/model/tcp-newreno.h``. Temos nesse arquivo:

::

  TracedValue<uint32_t> m_cWnd; //Congestion window

..
	You should now understand this code completely.  If we have a pointer to the 
	``TcpNewReno``, we can ``TraceConnect`` to the "CongestionWindow" trace 
	source if we provide an appropriate callback target.  This is the same kind of
	trace source that we saw in the simple example at the start of this section,
	except that we are talking about ``uint32_t`` instead of ``int32_t``.

Voc� deveria entender este c�digo. Se n�s temos um ponteiro para ``TcpNewReno``, podemos fazer ``TraceConnect`` para a origem do rastreamento "CongestionWindow" se fornecermos uma *callback* adequada. � o mesmo tipo de origem do rastreamento que n�s abordamos no exemplo simples no in�cio da se��o, exceto que estamos usando ``uint32_t`` ao inv�s de ``int32_t``.

..
	We now know that we need to provide a callback that returns void and takes 
	two ``uint32_t`` parameters, the first being the old value and the second 
	being the new value:

Precisamos prover uma *callback* que retorne ``void`` e receba dois par�metros ``uint32_t``, o primeiro representando o valor antigo e o segundo o novo valor:

::

  void
  CwndTrace (uint32_t oldValue, uint32_t newValue)
  {
    ...
  }

.. 
	What Script to Use?

Qual c�digo Usar?
+++++++++++++++++

..
	It's always best to try and find working code laying around that you can 
	modify, rather than starting from scratch.  So the first order of business now
	is to find some code that already hooks the "CongestionWindow" trace source
	and see if we can modify it.  As usual, grep is your friend:

� sempre melhor localizar e modificar um c�digo operacional que iniciar do zero. Portanto, vamos procurar uma origem do rastreamento da "CongestionWindow" e verificar se � poss�vel modificar. Para tal, usamos novamente o *grep*:

::

  find . -name '*.cc' | xargs grep CongestionWindow

..
	This will point out a couple of promising candidates: 
	``examples/tcp/tcp-large-transfer.cc`` and 
	``src/test/ns3tcp/ns3tcp-cwnd-test-suite.cc``.

Encontramos alguns candidatos:
``examples/tcp/tcp-large-transfer.cc`` e
``src/test/ns3tcp/ns3tcp-cwnd-test-suite.cc``.

..
	We haven't visited any of the test code yet, so let's take a look there.  You
	will typically find that test code is fairly minimal, so this is probably a
	very good bet.  Open ``src/test/ns3tcp/ns3tcp-cwnd-test-suite.cc`` in your
	favorite editor and search for "CongestionWindow".  You will find,

N�s n�o visitamos nenhum c�digo de teste ainda, ent�o vamos fazer isto agora. C�digo de teste � pequeno, logo � uma �tima escolha. Acesse o arquivo ``src/test/ns3tcp/ns3tcp-cwnd-test-suite.cc`` e localize "CongestionWindow". Como resultado, temos

::

  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", 
    MakeCallback (&Ns3TcpCwndTestCase1::CwndChange, this));

..
	This should look very familiar to you.  We mentioned above that if we had a
	"CongestionWindow" trace source.  That's exactly what we have here; so it
	pointer to the ``TcpNewReno``, we could ``TraceConnect`` to the 
	turns out that this line of code does exactly what we want.  Let's go ahead
	and extract the code we need from this function 
	(``Ns3TcpCwndTestCase1::DoRun (void)``).  If you look at this function,
	you will find that it looks just like an |ns3| script.  It turns out that
	is exactly what it is.  It is a script run by the test framework, so we can just
	pull it out and wrap it in ``main`` instead of in ``DoRun``.  Rather than
	walk through this, step, by step, we have provided the file that results from
	porting this test back to a native |ns3| script --
	``examples/tutorial/fifth.cc``.  

Como abordado, temos uma origem do rastreamento  "CongestionWindow"; ent�o ela aponta para ``TcpNewReno``, poder�amos alterar o ``TraceConnect`` para o que n�s desejamos. Vamos extrair o c�digo que precisamos desta fun��o (``Ns3TcpCwndTestCase1::DoRun (void)``). Se voc� observar, perceber� que parece como um c�digo |ns3|. E descobre-se exatamente que realmente � um c�digo. � um c�digo executado pelo `framework` de teste, logo podemos apenas coloc�-lo no ``main`` ao inv�s de ``DoRun``.  A tradu��o deste teste para um c�digo nativo do |ns3| � apresentada no arquivo ``examples/tutorial/fifth.cc``.


.. 
	A Common Problem and Solution

Um Problema Comum e a Solu��o
+++++++++++++++++++++++++++++

..
	The ``fifth.cc`` example demonstrates an extremely important rule that you 
	must understand before using any kind of ``Attribute``:  you must ensure 
	that the target of a ``Config`` command exists before trying to use it.
	This is no different than saying an object must be instantiated before trying
	to call it.  Although this may seem obvious when stated this way, it does
	trip up many people trying to use the system for the first time.

O exemplo ``fifth.cc`` demonstra um importante regra que devemos entender antes de usar qualquer tipo de  Atributo: devemos garantir que o alvo de um comando ``Config`` existe antes de tentar us�-lo. � a mesma ideia que um objeto n�o pode ser usado sem ser primeiro instanciado. Embora pare�a �bvio, muitas pessoas erram ao usar o sistema pela primeira vez.

..
	Let's return to basics for a moment.  There are three basic time periods that
	exist in any |ns3| script.  The first time period is sometimes called 
	"Configuration Time" or "Setup Time," and is in force during the period 
	when the ``main`` function of your script is running, but before 
	``Simulator::Run`` is called.  The second time period  is sometimes called
	"Simulation Time" and is in force during the time period when 
	``Simulator::Run`` is actively executing its events.  After it completes
	executing the simulation,  ``Simulator::Run`` will return control back to 
	the ``main`` function.  When this happens, the script enters what can be 
	called "Teardown Time," which is when the structures and objects created 
	during setup and taken apart and released.

H� tr�s fases b�sicas em qualquer c�digo |ns3|. A primeira � a chamada de "Configuration Time" ou "Setup Time" e ocorre durante a execu��o da fun��o ``main``, mas antes da chamada ``Simulator::Run``. O segunda fase � chamada de "Simulation Time" e � quando o ``Simulator::Run`` est� executando seus eventos. Ap�s completar a execu��o da simula��o, ``Simulator::Run`` devolve o controle a fun��o ``main``. Quando isto acontece, o c�digo entra na terceira fase, o "Teardown Time", que  � quando estruturas e objetos criados durante a configura��o s�o analisados e liberados.

..
	Perhaps the most common mistake made in trying to use the tracing system is 
	assuming that entities constructed dynamically during simulation time are
	available during configuration time.  In particular, an |ns3|
	``Socket`` is a dynamic object often created by ``Applications`` to
	communicate between ``Nodes``.  An |ns3| ``Application`` 
	always has a "Start Time" and a "Stop Time" associated with it.  In the
	vast majority of cases, an ``Application`` will not attempt to create 
	a dynamic object until its ``StartApplication`` method is called at some
	"Start Time".  This is to ensure that the simulation is completely 
	configured before the app tries to do anything (what would happen if it tried
	to connect to a node that didn't exist yet during configuration time). 
	The answer to this issue is to 1) create a simulator event that is run after the 
	dynamic object is created and hook the trace when that event is executed; or
	2) create the dynamic object at configuration time, hook it then, and give 
	the object to the system to use during simulation time.  We took the second 
	approach in the ``fifth.cc`` example.  This decision required us to create
	the ``MyApp`` ``Application``, the entire purpose of which is to take 
	a ``Socket`` as a parameter.  

Talvez o erro mais comum em tentar usar o sistema de rastreamento � supor que entidades constru�das dinamicamente durante a fase de simula��o est�o acess�veis durante  a fase de configura��o. Em particular, um ``Socket`` |ns3| � um objeto din�mico frequentemente criado por Aplica��es (``Applications``) para comunica��o entre n�s de redes.  Uma Aplica��o |ns3| tem um "Start Time" e "Stop Time" associado a ela. Na maioria dos casos, uma Aplica��o n�o tentar criar um objeto din�mico at� que seu m�todo ``StartApplication`` � chamado
em algum "Start Time". Isto � para garantir que a simula��o est� completamente configurada antes que a aplica��o tente fazer alguma coisa (o que aconteceria se tentasse conectar a um n� que n�o existisse durante a fase de configura��o). A resposta para esta quest�o �:
	
1. criar um evento no simulador que � executado depois que o objeto din�mico 
   � criado e ativar o rastreador quando aquele evento � executado; ou 
2. criar o objeto din�mico na fase de configura��o, ativ�-lo,
   e passar o objeto para o sistema usar durante a fase de simula��o. 

N�s consideramos a segunda abordagem no exemplo ``fifth.cc``. A decis�o implicou na cria��o da Aplica��o ``MyApp``, com o prop�sito de passar um ``Socket`` como par�metro.

.. 
	A fifth.cc Walkthrough

Analisando o exemplo fifth.cc
+++++++++++++++++++++++++++++

..
	Now, let's take a look at the example program we constructed by dissecting
	the congestion window test.  Open ``examples/tutorial/fifth.cc`` in your
	favorite editor.  You should see some familiar looking code:

Agora, vamos analisar o programa exemplo detalhando o teste da janela de congestionamento.
Segue o c�digo do arquivo localizado em ``examples/tutorial/fifth.cc``:

::

  /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
  /*
   * This program is free software; you can redistribute it and/or modify
   * it under the terms of the GNU General Public License version 2 as
   * published by the Free Software Foundation;
   *
   * This program is distributed in the hope that it will be useful,
   * but WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU General Public License for more details.
   *
   * You should have received a copy of the GNU General Public License
   * along with this program; if not, write to the Free Software
   * Foundation, Include., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   */
  
  #include <fstream>
  #include "ns3/core-module.h"
  #include "ns3/network-module.h"
  #include "ns3/internet-module.h"
  #include "ns3/point-to-point-module.h"
  #include "ns3/applications-module.h"
  
  using namespace ns3;
  
  NS_LOG_COMPONENT_DEFINE ("FifthScriptExample");

..
	This has all been covered, so we won't rehash it.  The next lines of source are
	the network illustration and a comment addressing the problem described above
	with ``Socket``.

Todo o c�digo apresentado j� foi discutido. As pr�ximas linhas s�o coment�rios apresentando
a estrutura da rede e coment�rios abordando o problema descrito com o ``Socket``.

::

  // ===========================================================================
  //
  //         node 0                 node 1
  //   +----------------+    +----------------+
  //   |    ns-3 TCP    |    |    ns-3 TCP    |
  //   +----------------+    +----------------+
  //   |    10.1.1.1    |    |    10.1.1.2    |
  //   +----------------+    +----------------+
  //   | point-to-point |    | point-to-point |
  //   +----------------+    +----------------+
  //           |                     |
  //           +---------------------+
  //                5 Mbps, 2 ms
  //
  //
  // We want to look at changes in the ns-3 TCP congestion window.  We need
  // to crank up a flow and hook the CongestionWindow attribute on the socket
  // of the sender.  Normally one would use an on-off application to generate a
  // flow, but this has a couple of problems.  First, the socket of the on-off
  // application is not created until Application Start time, so we wouldn't be
  // able to hook the socket (now) at configuration time.  Second, even if we
  // could arrange a call after start time, the socket is not public so we
  // couldn't get at it.
  //
  // So, we can cook up a simple version of the on-off application that does what
  // we want.  On the plus side we don't need all of the complexity of the on-off
  // application.  On the minus side, we don't have a helper, so we have to get
  // a little more involved in the details, but this is trivial.
  //
  // So first, we create a socket and do the trace connect on it; then we pass
  // this socket into the constructor of our simple application which we then
  // install in the source node.
  // ===========================================================================
  //

.. 
	This should also be self-explanatory.  

..
	The next part is the declaration of the ``MyApp`` ``Application`` that
	we put together to allow the ``Socket`` to be created at configuration time.

A pr�xima parte � a declara��o da Aplica��o ``MyApp`` que permite que o ``Socket`` seja criado na fase de configura��o.

::

  class MyApp : public Application
  {
  public:
  
    MyApp ();
    virtual ~MyApp();
  
    void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, 
      uint32_t nPackets, DataRate dataRate);
  
  private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);
  
    void ScheduleTx (void);
    void SendPacket (void);
  
    Ptr<Socket>     m_socket;
    Address         m_peer;
    uint32_t        m_packetSize;
    uint32_t        m_nPackets;
    DataRate        m_dataRate;
    EventId         m_sendEvent;
    bool            m_running;
    uint32_t        m_packetsSent;
  };

..
	You can see that this class inherits from the |ns3| ``Application``
	class.  Take a look at ``src/network/model/application.h`` if you are interested in 
	what is inherited.  The ``MyApp`` class is obligated to override the 
	``StartApplication`` and ``StopApplication`` methods.  These methods are
	automatically called when ``MyApp`` is required to start and stop sending
	data during the simulation.

A classe ``MyApp`` herda a classe ``Application`` do |ns3|. Acesse  o arquivo ``src/network/model/application.h`` se estiver interessado sobre detalhes dessa heran�a. A classe ``MyApp`` � obrigada sobrescrever os m�todos ``StartApplication`` e ``StopApplication``. Estes m�todos s�o automaticamente chamado quando ``MyApp`` � solicitada iniciar e parar de enviar dados durante a simula��o.

.. 
	How Applications are Started and Stopped (optional)

Como Aplica��es s�o Iniciadas e Paradas (Opcional)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

..
	It is worthwhile to spend a bit of time explaining how events actually get 
	started in the system.  This is another fairly deep explanation, and can be
	ignored if you aren't planning on venturing down into the guts of the system.
	It is useful, however, in that the discussion touches on how some very important
	parts of |ns3| work and exposes some important idioms.  If you are 
	planning on implementing new models, you probably want to understand this
	section.

Nesta se��o � explicado como eventos tem in�cio no sistema. � uma explica��o mais detalhada e n�o � necess�ria se n�o planeja entender detalhes do sistema. � interessante, por outro lado, pois aborda como partes do |ns3| trabalham e mostra alguns detalhes de implementa��o importantes. Se voc� planeja implementar novos modelos, ent�o deve entender essa se��o.

..
	The most common way to start pumping events is to start an ``Application``.
	This is done as the result of the following (hopefully) familar lines of an 
	|ns3| script:

A maneira mais comum de iniciar eventos � iniciar uma Aplica��o. Segue as linhas de um c�digo |ns3| que faz exatamente isso:

::

  ApplicationContainer apps = ...
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

..
	The application container code (see ``src/network/helper/application-container.h`` if
	you are interested) loops through its contained applications and calls,

O c�digo do cont�iner aplica��o (``src/network/helper/application-container.h``) itera pelas aplica��es no cont�iner e chama,

::

  app->SetStartTime (startTime);

.. 
	as a result of the ``apps.Start`` call and

como um resultado da chamada ``apps.Start`` e

::

  app->SetStopTime (stopTime);

.. 
	as a result of the ``apps.Stop`` call.

como um resultado da chamada  ``apps.Stop``.

..
	The ultimate result of these calls is that we want to have the simulator 
	automatically make calls into our ``Applications`` to tell them when to
	start and stop.  In the case of ``MyApp``, it inherits from class
	``Application`` and overrides ``StartApplication``, and 
	``StopApplication``.  These are the functions that will be called by
	the simulator at the appropriate time.  In the case of ``MyApp`` you
	will find that ``MyApp::StartApplication`` does the initial ``Bind``,
	and ``Connect`` on the socket, and then starts data flowing by calling
	``MyApp::SendPacket``.  ``MyApp::StopApplication`` stops generating
	packets by cancelling any pending send events and closing the socket.

O �ltimo resultado destas chamadas queremos ter o simulador executando chamadas em nossa ``Applications`` para controlar o inicio e a parada. No caso ``MyApp``, herda da classe ``Application`` e sobrescreve ``StartApplication`` e ``StopApplication``. Estas s�o as fun��es invocadas pelo simulador no momento certo. No caso de ``MyApp``, o ``MyApp::StartApplication`` faz o ``Bind`` e ``Connect`` no `socket`, em seguida, inicia o fluxo de dados chamando ``MyApp::SendPacket``. ``MyApp::StopApplication`` interrompe a gera��o de pacotes cancelando qualquer evento pendente de envio e tamb�m fechando o socket.

..
	One of the nice things about |ns3| is that you can completely 
	ignore the implementation details of how your ``Application`` is 
	"automagically" called by the simulator at the correct time.  But since
	we have already ventured deep into |ns3| already, let's go for it.

Uma das coisas legais sobre o |ns3| � que podemos ignorar completamente os detalhes de implementa��o de como sua Aplica��o � "automaticamente" chamada pelo simulador no momento correto. De qualquer forma, detalhamos como isso acontece a seguir.

..
	If you look at ``src/network/model/application.cc`` you will find that the
	``SetStartTime`` method of an ``Application`` just sets the member 
	variable ``m_startTime`` and the ``SetStopTime`` method just sets 
	``m_stopTime``.  From there, without some hints, the trail will probably
	end.

Se observarmos em ``src/network/model/application.cc``, descobriremos que o 
m�todo ``SetStartTime`` de uma ``Application`` apenas altera a vari�vel ``m_startTime`` e o m�todo ``SetStopTime`` apenas altera a vari�vel ``m_stopTime``.  

..
	The key to picking up the trail again is to know that there is a global 
	list of all of the nodes in the system.  Whenever you create a node in 
	a simulation, a pointer to that node is added to the global ``NodeList``.

Para continuar e entender o processo, precisamos saber que h� uma lista global de todos os n�s no sistema. Sempre que voc� cria um n� em uma simula��o, um ponteiro para aquele n� � adicionado para a lista global ``NodeList``.

..
	Take a look at ``src/network/model/node-list.cc`` and search for 
	``NodeList::Add``.  The public static implementation calls into a private
	implementation called ``NodeListPriv::Add``.  This is a relatively common
	idom in |ns3|.  So, take a look at ``NodeListPriv::Add``.  There
	you will find,

Observe em ``src/network/model/node-list.cc`` e procure por ``NodeList::Add``. A implementa��o ``public static`` chama uma implementa��o privada denominada ``NodeListPriv::Add``. Isto � comum no |ns3|. Ent�o, observe ``NodeListPriv::Add`` e encontrar�,

::

  Simulator::ScheduleWithContext (index, TimeStep (0), &Node::Start, node);

..
	This tells you that whenever a ``Node`` is created in a simulation, as
	a side-effect, a call to that node's ``Start`` method is scheduled for
	you that happens at time zero.  Don't read too much into that name, yet.
	It doesn't mean that the node is going to start doing anything, it can be
	interpreted as an informational call into the ``Node`` telling it that 
	the simulation has started, not a call for action telling the ``Node``
	to start doing something.

Isto significa que sempre que um ``Node`` � criado em uma simula��o, como uma implica��o, uma chamada para o m�todo ``Start`` do n� � agendada para que ocorra no tempo zero. Isto n�o significa que o n� vai iniciar fazendo alguma coisa, pode ser interpretado como uma chamada informacional no ``Node`` dizendo a ele que a simula��o teve in�cio, n�o uma chamada para a��o dizendo ao ``Node`` iniciar alguma coisa.

..
	So, ``NodeList::Add`` indirectly schedules a call to ``Node::Start``
	at time zero to advise a new node that the simulation has started.  If you 
	look in ``src/network/model/node.h`` you will, however, not find a method called
	``Node::Start``.  It turns out that the ``Start`` method is inherited
	from class ``Object``.  All objects in the system can be notified when
	the simulation starts, and objects of class ``Node`` are just one kind
	of those objects.

Ent�o, o ``NodeList::Add`` indiretamente agenda uma chamada para ``Node::Start`` no tempo zero, para informar ao novo n� que a simula��o foi iniciada. Se olharmos em ``src/network/model/node.h`` n�o acharemos um m�todo chamado ``Node::Start``. Acontece que o m�todo ``Start`` � herdado da classe ``Object``. Todos objetos no sistema podem ser avisados que a simula��o iniciou e objetos da classe ``Node`` s�o exemplos.

..
	Take a look at ``src/core/model/object.cc`` next and search for ``Object::Start``.
	This code is not as straightforward as you might have expected since 
	|ns3| ``Objects`` support aggregation.  The code in 
	``Object::Start`` then loops through all of the objects that have been
	aggregated together and calls their ``DoStart`` method.  This is another
	idiom that is very common in |ns3|.  There is a public API method,
	that stays constant across implementations, that calls a private implementation
	method that is inherited and implemented by subclasses.  The names are typically
	something like ``MethodName`` for the public API and ``DoMethodName`` for
	the private API.

Observe em seguida ``src/core/model/object.cc``. Localize por ``Object::Start``. Este c�digo n�o � t�o simples como voc� esperava desde que ``Objects`` |ns3| suportam agrega��o. O c�digo em ``Object::Start`` ent�o percorre todos os objetos que est�o agregados e chama o m�todo ``DoStart`` de cada um. Este � uma outra pr�tica muito comum em |ns3|. H� um m�todo p�blica na API, que permanece constante entre implementa��es, que chama um m�todo de implementa��o privada que � herdado e implementado por subclasses. Os nomes s�o tipicamente
algo como ``MethodName`` para os da API p�blica e ``DoMethodName`` para os da API privada.

..
	This tells us that we should look for a ``Node::DoStart`` method in 
	``src/network/model/node.cc`` for the method that will continue our trail.  If you
	locate the code, you will find a method that loops through all of the devices
	in the node and then all of the applications in the node calling 
	``device->Start`` and ``application->Start`` respectively.

Logo, dever�amos procurar por um m�todo ``Node::DoStart`` em  ``src/network/model/node.cc``. Ao localizar o m�todo, descobrir� um m�todo que percorre todos os dispositivos e aplica��es no n� chamando respectivamente ``device->Start`` e ``application->Start``.

..
	You may already know that classes ``Device`` and ``Application`` both
	inherit from class ``Object`` and so the next step will be to look at
	what happens when ``Application::DoStart`` is called.  Take a look at
	``src/network/model/application.cc`` and you will find:

As classes ``Device`` e ``Application`` herdam da classe ``Object``, ent�o o pr�ximo passo � entender o que acontece quando ``Application::DoStart`` � executado. Observe o c�digo em ``src/network/model/application.cc``:

::

  void
  Application::DoStart (void)
  {
    m_startEvent = Simulator::Schedule (m_startTime, &Application::StartApplication, this);
    if (m_stopTime != TimeStep (0))
      {
        m_stopEvent = Simulator::Schedule (m_stopTime, &Application::StopApplication, this);
      }
    Object::DoStart ();
  }

..
	Here, we finally come to the end of the trail.  If you have kept it all straight,
	when you implement an |ns3| ``Application``, your new application 
	inherits from class ``Application``.  You override the ``StartApplication``
	and ``StopApplication`` methods and provide mechanisms for starting and 
	stopping the flow of data out of your new ``Application``.  When a ``Node``
	is created in the simulation, it is added to a global ``NodeList``.  The act
	of adding a node to this ``NodeList`` causes a simulator event to be scheduled
	for time zero which calls the ``Node::Start`` method of the newly added 
	``Node`` to be called when the simulation starts.  Since a ``Node`` inherits
	from ``Object``, this calls the ``Object::Start`` method on the ``Node``
	which, in turn, calls the ``DoStart`` methods on all of the ``Objects``
	aggregated to the ``Node`` (think mobility models).  Since the ``Node``
	``Object`` has overridden ``DoStart``, that method is called when the 
	simulation starts.  The ``Node::DoStart`` method calls the ``Start`` methods
	of all of the ``Applications`` on the node.  Since ``Applications`` are
	also ``Objects``, this causes ``Application::DoStart`` to be called.  When
	``Application::DoStart`` is called, it schedules events for the 
	``StartApplication`` and ``StopApplication`` calls on the ``Application``.
	These calls are designed to start and stop the flow of data from the 
	``Application``

Aqui finalizamos nosso detalhamento. Ao implementar uma Aplica��o do |ns3|, sua nova aplica��o herda da classe ``Application``. Voc� sobrescreve os m�todos ``StartApplication`` e ``StopApplication`` e prov� mecanismos para iniciar e finalizar o fluxo de dados de sua nova ``Application``. Quando um ``Node`` � criado na simula��o, ele � adicionado a uma lista global ``NodeList``. A a��o de adicionar um n� na lista faz com que um evento do simulador seja agendado para o tempo zero e que chama o m�todo ``Node::Start`` do ``Node`` recentemente adicionado para ser chamado quando a simula��o inicia. Como um ``Node`` herda de ``Object``,
a chamada invoca o m�todo ``Object::Start`` no ``Node``, o qual, por sua vez, chama os m�todos ``DoStart`` em todos os ``Objects`` agregados ao ``Node`` (pense em modelos m�veis).  Como o ``Node`` ``Object`` 
tem sobrescritos ``DoStart``, o m�todo � chamado quando a simula��o inicia. O m�todo ``Node::DoStart`` chama o m�todo ``Start`` de todas as ``Applications`` no n�. Por sua vez, ``Applications`` s�o tamb�m ``Objects``, o que resulta na invoca��o do ``Application::DoStart``.  Quando ``Application::DoStart`` � chamada, ela agenda eventos para as chamadas ``StartApplication`` e ``StopApplication`` na ``Application``. Estas chamadas s�o projetadas para iniciar e parar o fluxo de dados da ``Application``.

..
	This has been another fairly long journey, but it only has to be made once, and
	you now understand another very deep piece of |ns3|.

Ap�s essa longa jornada, voc� pode entende melhor outra parte do |ns3|.

..
	The MyApp Application

A Aplica��o MyApp
~~~~~~~~~~~~~~~~~

..
	The ``MyApp`` ``Application`` needs a constructor and a destructor,
	of course:

A Aplica��o ``MyApp`` precisa de um construtor e um destrutor,

::

  MyApp::MyApp ()
    : m_socket (0),
      m_peer (),
      m_packetSize (0),
      m_nPackets (0),
      m_dataRate (0),
      m_sendEvent (),
      m_running (false),
      m_packetsSent (0)
  {
  }
  
  MyApp::~MyApp()
  {
    m_socket = 0;
  }

..
	The existence of the next bit of code is the whole reason why we wrote this
	``Application`` in the first place.

O c�digo seguinte � a principal raz�o da exist�ncia desta Aplica��o.

::

  void
  MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, 
                       uint32_t nPackets, DataRate dataRate)
  {
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
  }

..  
	This code should be pretty self-explanatory.  We are just initializing member
	variables.  The important one from the perspective of tracing is the 
	``Ptr<Socket> socket`` which we needed to provide to the application 
	during configuration time.  Recall that we are going to create the ``Socket``
	as a ``TcpSocket`` (which is implemented by ``TcpNewReno``) and hook 
	its "CongestionWindow" trace source before passing it to the ``Setup``
	method.

Neste c�digo inicializamos os atributos da classe. Do ponto de vista do rastreamento, a mais importante � ``Ptr<Socket> socket`` que deve ser passado para a aplica��o durante o fase de configura��o. Lembre-se que vamos criar o ``Socket`` como um ``TcpSocket`` (que � implementado por ``TcpNewReno``) e associar sua origem do rastreamento de sua *"CongestionWindow"* antes de pass�-lo no m�todo ``Setup``.

::

  void
  MyApp::StartApplication (void)
  {
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind ();
    m_socket->Connect (m_peer);
    SendPacket ();
  }

..
	The above code is the overridden implementation ``Application::StartApplication``
	that will be automatically called by the simulator to start our ``Application``
	running at the appropriate time.  You can see that it does a ``Socket`` ``Bind``
	operation.  If you are familiar with Berkeley Sockets this shouldn't be a surprise.
	It performs the required work on the local side of the connection just as you might 
	expect.  The following ``Connect`` will do what is required to establish a connection 
	with the TCP at ``Address`` m_peer.  It should now be clear why we need to defer
	a lot of this to simulation time, since the ``Connect`` is going to need a fully
	functioning network to complete.  After the ``Connect``, the ``Application`` 
	then starts creating simulation events by calling ``SendPacket``.

Este c�digo sobrescreve ``Application::StartApplication`` que ser� chamado automaticamente pelo simulador para iniciar a  ``Application`` no momento certo. Observamos que � realizada uma opera��o ``Socket`` ``Bind``. Se voc� conhece Sockets de Berkeley isto n�o � uma novidade. � respons�vel pelo conex�o no lado do cliente, ou seja, o ``Connect`` estabelece uma  conex�o usando TCP no endere�o ``m_peer``. Por isso, precisamos de uma infraestrutura funcional de rede antes de executar a fase de simula��o. Depois do ``Connect``, a ``Application`` inicia a cria��o dos eventos de simula��o chamando ``SendPacket``.

::

  void
  MyApp::StopApplication (void)
  {
    m_running = false;
  
    if (m_sendEvent.IsRunning ())
      {
        Simulator::Cancel (m_sendEvent);
      }
  
    if (m_socket)
      {
        m_socket->Close ();
      }
  }

..
	Every time a simulation event is scheduled, an ``Event`` is created.  If the 
	``Event`` is pending execution or executing, its method ``IsRunning`` will
	return ``true``.  In this code, if ``IsRunning()`` returns true, we 
	``Cancel`` the event which removes it from the simulator event queue.  By 
	doing this, we break the chain of events that the ``Application`` is using to
	keep sending its ``Packets`` and the ``Application`` goes quiet.  After we 
	quiet the ``Application`` we ``Close`` the socket which tears down the TCP 
	connection.

A todo instante um evento da simula��o � agendado, isto �, um ``Event`` � criado. Se o ``Event`` � uma execu��o pendente ou est� executando, seu m�todo ``IsRunning`` retornar� ``true``. Neste c�digo, se ``IsRunning()`` retorna verdadeiro (`true`), n�s cancelamos (``Cancel``) o evento, e por consequ�ncia, � removido da fila de eventos do simulador. Dessa forma, interrompemos a cadeia de eventos que a 
``Application`` est� usando para enviar seus ``Packets``. A Aplica��o n�o enviar� mais pacotes e em seguida fechamos (``Close``) o `socket` encerrando a conex�o TCP.

..
	The socket is actually deleted in the destructor when the ``m_socket = 0`` is
	executed.  This removes the last reference to the underlying Ptr<Socket> which 
	causes the destructor of that Object to be called.

O socket � deletado no destrutor quando ``m_socket = 0`` � executado. Isto remove a �ltima refer�ncia para Ptr<Socket>  que ocasiona o destrutor daquele Objeto ser chamado.

..
	Recall that ``StartApplication`` called ``SendPacket`` to start the 
	chain of events that describes the ``Application`` behavior.

Lembre-se que ``StartApplication`` chamou ``SendPacket`` para iniciar a cadeia de eventos que descreve o comportamento da ``Application``.

::

  void
  MyApp::SendPacket (void)
  {
    Ptr<Packet> packet = Create<Packet> (m_packetSize);
    m_socket->Send (packet);
  
    if (++m_packetsSent < m_nPackets)
      {
        ScheduleTx ();
      }
  }

..
	Here, you see that ``SendPacket`` does just that.  It creates a ``Packet``
	and then does a ``Send`` which, if you know Berkeley Sockets, is probably 
	just what you expected to see.

Este c�digo apenas cria um pacote (``Packet``) e ent�o envia (``Send``).

..
	It is the responsibility of the ``Application`` to keep scheduling the 
	chain of events, so the next lines call ``ScheduleTx`` to schedule another
	transmit event (a ``SendPacket``) until the ``Application`` decides it
	has sent enough.

� responsabilidade da ``Application`` gerenciar o agendamento da cadeia de eventos, ent�o, a chamada ``ScheduleTx`` agenda outro evento de transmiss�o (um ``SendPacket``) at� que a ``Application`` decida que enviou o suficiente.

::

  void
  MyApp::ScheduleTx (void)
  {
    if (m_running)
      {
        Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
        m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
      }
  }

..
	Here, you see that ``ScheduleTx`` does exactly that.  If the ``Application``
	is running (if ``StopApplication`` has not been called) it will schedule a 
	new event, which calls ``SendPacket`` again.  The alert reader will spot
	something that also trips up new users.  The data rate of an ``Application`` is
	just that.  It has nothing to do with the data rate of an underlying ``Channel``.
	This is the rate at which the ``Application`` produces bits.  It does not take
	into account any overhead for the various protocols or channels that it uses to 
	transport the data.  If you set the data rate of an ``Application`` to the same
	data rate as your underlying ``Channel`` you will eventually get a buffer overflow.

Enquanto a ``Application`` est� executando, ``ScheduleTx`` agendar� um novo evento, que chama ``SendPacket`` novamente. Verifica-se que a taxa de transmiss�o � sempre a mesma, ou seja, � a taxa que a ``Application`` produz os bits. N�o considera nenhuma sobrecarga de protocolos ou canais f�sicos no transporte dos dados. Se alterarmos a taxa de transmiss�o da ``Application`` para a mesma taxa dos canais f�sicos,  poderemos
ter um estouro de *buffer*.

.. 
	The Trace Sinks

Destino do Rastreamento
~~~~~~~~~~~~~~~~~~~~~~~

..
	The whole point of this exercise is to get trace callbacks from TCP indicating the
	congestion window has been updated.  The next piece of code implements the 
	corresponding trace sink:

O foco deste exerc�cio � obter notifica��es (*callbacks*) do TCP indicando a modifica��o da janela de congestionamento. O c�digo a seguir implementa o destino do rastreamento.

::

  static void
  CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
  {
    NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  }

..
	This should be very familiar to you now, so we won't dwell on the details.  This
	function just logs the current simulation time and the new value of the 
	congestion window every time it is changed.  You can probably imagine that you
	could load the resulting output into a graphics program (gnuplot or Excel) and
	immediately see a nice graph of the congestion window behavior over time.

Esta fun��o registra o tempo de simula��o atual e o novo valor da janela de congestionamento toda vez que � modificada. Poder�amos usar essa sa�da para construir um gr�fico  do comportamento da janela de congestionamento com rela��o ao tempo.

..
	We added a new trace sink to show where packets are dropped.  We are going to 
	add an error model to this code also, so we wanted to demonstrate this working.

N�s adicionamos um novo destino do rastreamento para mostrar onde pacotes s�o perdidos. Vamos adicionar um modelo de erro.

::

  static void
  RxDrop (Ptr<const Packet> p)
  {
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  }

..
	This trace sink will be connected to the "PhyRxDrop" trace source of the 
	point-to-point NetDevice.  This trace source fires when a packet is dropped
	by the physical layer of a ``NetDevice``.  If you take a small detour to the
	source (``src/point-to-point/model/point-to-point-net-device.cc``) you will
	see that this trace source refers to ``PointToPointNetDevice::m_phyRxDropTrace``.
	If you then look in ``src/point-to-point/model/point-to-point-net-device.h``
	for this member variable, you will find that it is declared as a
	``TracedCallback<Ptr<const Packet> >``.  This should tell you that the
	callback target should be a function that returns void and takes a single
	parameter which is a ``Ptr<const Packet>`` -- just what we have above.

Este destino do rastreamento ser� conectado a origem do rastreamento "PhyRxDrop" do ``NetDevice`` ponto-a-ponto. Esta origem do rastreamento dispara quando um pacote � removido da camada f�sica de um ``NetDevice``. Se olharmos rapidamente ``src/point-to-point/model/point-to-point-net-device.cc`` verificamos que a origem do rastreamento refere-se a ``PointToPointNetDevice::m_phyRxDropTrace``. E se procurarmos em ``src/point-to-point/model/point-to-point-net-device.h`` por essa vari�vel, encontraremos que ela est� declarada como uma ``TracedCallback<Ptr<const Packet> >``. Isto significa que nosso *callback* deve ser uma fun��o que retorna ``void`` e tem um �nico par�metro ``Ptr<const Packet>``.


.. 
	The Main Program

O Programa Principal
~~~~~~~~~~~~~~~~~~~~

.. 
	The following code should be very familiar to you by now:

O c�digo a seguir corresponde ao in�cio da fun��o principal:

::

  int
  main (int argc, char *argv[])
  {
    NodeContainer nodes;
    nodes.Create (2);
  
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  
    NetDeviceContainer devices;
    devices = pointToPoint.Install (nodes);

..
	This creates two nodes with a point-to-point channel between them, just as
	shown in the illustration at the start of the file.

S�o criados dois n�s ligados por um canal ponto-a-ponto, como mostrado na ilustra��o
no in�cio do arquivo.

..
	The next few lines of code show something new.  If we trace a connection that
	behaves perfectly, we will end up with a monotonically increasing congestion
	window.  To see any interesting behavior, we really want to introduce link 
	errors which will drop packets, cause duplicate ACKs and trigger the more
	interesting behaviors of the congestion window.


Nas pr�ximas linhas, temos um c�digo com algumas informa��es novas. Se n�s 

rastrearmos uma conex�o que comporta-se perfeitamente, terminamos com um

janela de congestionamento que aumenta monoliticamente. Para observarmos um

comportamento interessante, introduzimos erros que causar�o perda de pacotes,

duplica��o de ACK's e assim, introduz comportamentos mais interessantes a

janela de congestionamento.


..
	|ns3| provides ``ErrorModel`` objects which can be attached to
	``Channels``.  We are using the ``RateErrorModel`` which allows us
	to introduce errors into a ``Channel`` at a given *rate*. 


O |ns3| prov� objetos de um modelo de erros (``ErrorModel``) que pode ser adicionado aos canais (``Channels``). N�s usamos o ``RateErrorModel`` que permite introduzir erros no canal dada uma *taxa*.

::

  Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> (
    "RanVar", RandomVariableValue (UniformVariable (0., 1.)),
    "ErrorRate", DoubleValue (0.00001));
  devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

..
	The above code instantiates a ``RateErrorModel`` Object.  Rather than 
	using the two-step process of instantiating it and then setting Attributes,
	we use the convenience function ``CreateObjectWithAttributes`` which
	allows us to do both at the same time.  We set the "RanVar" 
	``Attribute`` to a random variable that generates a uniform distribution
	from 0 to 1.  We also set the "ErrorRate" ``Attribute``.
	We then set the resulting instantiated ``RateErrorModel`` as the error
	model used by the point-to-point ``NetDevice``.  This will give us some
	retransmissions and make our plot a little more interesting.

O c�digo instancia um objeto ``RateErrorModel``. Para simplificar usamos a fun��o ``CreateObjectWithAttributes`` que instancia e configura os Atributos. O Atributo "RanVar" foi configurado para uma vari�vel rand�mica que gera uma distribui��o uniforme entre 0 e 1. O Atributo "ErrorRate" tamb�m foi alterado. Por fim, configuramos o modelo erro no ``NetDevice`` ponto-a-ponto modificando o atributo "ReceiveErrorModel".  Isto causar� retransmiss�es e o gr�fico ficar� mais interessante.

::

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

..
	The above code should be familiar.  It installs internet stacks on our two
	nodes and creates interfaces and assigns IP addresses for the point-to-point
	devices.

Neste c�digo configura a pilha de protocolos da internet nos dois n�s de rede, cria interfaces e associa endere�os IP para os dispositivos ponto-a-ponto.

..
	Since we are using TCP, we need something on the destination node to receive
	TCP connections and data.  The ``PacketSink`` ``Application`` is commonly
	used in |ns3| for that purpose.

Como estamos usando TCP, precisamos de um n� de destino para receber as conex�es e os dados.  O ``PacketSink`` ``Application``  � comumente usado no |ns3| para este prop�sito.

::

  uint16_t sinkPort = 8080;
  Address sinkAddress (InetSocketAddress(interfaces.GetAddress (1), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", 
    InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (20.));

.. 
	This should all be familiar, with the exception of,

Este c�digo deveria ser familiar, com exce��o de,

::

  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", 
    InetSocketAddress (Ipv4Address::GetAny (), sinkPort));

..
	This code instantiates a ``PacketSinkHelper`` and tells it to create sockets
	using the class ``ns3::TcpSocketFactory``.  This class implements a design 
	pattern called "object factory" which is a commonly used mechanism for 
	specifying a class used to create objects in an abstract way.  Here, instead of 
	having to create the objects themselves, you provide the ``PacketSinkHelper``
	a string that specifies a ``TypeId`` string used to create an object which 
	can then be used, in turn, to create instances of the Objects created by the 
	factory.

Este c�digo instancia um ``PacketSinkHelper`` e cria sockets usando a classe ``ns3::TcpSocketFactory``. Esta classe implementa o padr�o de projeto "f�brica de objetos". Dessa forma, em vez de criar os objetos diretamente, fornecemos ao ``PacketSinkHelper`` um texto que especifica um ``TypeId`` usado para criar
um objeto que, por sua vez, pode ser usado para criar inst�ncias de Objetos criados pela implementa��o da f�brica de objetos.

..
	The remaining parameter tells the ``Application`` which address and port it
	should ``Bind`` to.

O par�metro seguinte especifica o endere�o e a porta para o mapeamento.

.. 
	The next two lines of code will create the socket and connect the trace source.

As pr�ximas duas linhas do c�digo criam o `socket` e conectam a origem do rastreamento.

::

  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), 
    TcpSocketFactory::GetTypeId ());
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", 
    MakeCallback (&CwndChange));

..
	The first statement calls the static member function ``Socket::CreateSocket``
	and provides a ``Node`` and an explicit ``TypeId`` for the object factory
	used to create the socket.  This is a slightly lower level call than the 
	``PacketSinkHelper`` call above, and uses an explicit C++ type instead of 
	one referred to by a string.  Otherwise, it is conceptually the same thing.

A primeira declara��o chama a fun��o est�tica ``Socket::CreateSocket`` e passa um ``Node`` e um ``TypeId`` para o objeto f�brica usado para criar o `socket`. 

..
	Once the ``TcpSocket`` is created and attached to the ``Node``, we can
	use ``TraceConnectWithoutContext`` to connect the CongestionWindow trace 
	source to our trace sink.

Uma vez que o ``TcpSocket`` � criado e adicionado ao ``Node``, n�s usamos ``TraceConnectWithoutContext`` para conectar a origem do rastreamento "CongestionWindow" para o nosso destino do rastreamento.

..
	Recall that we coded an ``Application`` so we could take that ``Socket``
	we just made (during configuration time) and use it in simulation time.  We now 
	have to instantiate that ``Application``.  We didn't go to any trouble to
	create a helper to manage the ``Application`` so we are going to have to 
	create and install it "manually".  This is actually quite easy:

Codificamos uma ``Application`` ent�o podemos obter um ``Socket`` (durante a fase de configura��o) e usar na fase de simula��o. Temos agora que instanciar a ``Application``. Para tal, segue os passos:

::

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000, DataRate ("1Mbps"));
  nodes.Get (0)->AddApplication (app);
  app->Start (Seconds (1.));
  app->Stop (Seconds (20.));

..
	The first line creates an ``Object`` of type ``MyApp`` -- our
	``Application``.  The second line tells the ``Application`` what
	``Socket`` to use, what address to connect to, how much data to send 
	at each send event, how many send events to generate and the rate at which
	to produce data from those events.

A primeira linha cria um Objeto do tipo ``MyApp`` -- nossa ``Application``. A segunda linha especifica o `socket`, o endere�o de conex�o, a quantidade de dados a ser enviada em cada evento, a quantidade de eventos de transmiss�o a ser gerados e a taxa de produ��o de dados para estes eventos.

	Next, we manually add the ``MyApp Application`` to the source node
	and explicitly call the ``Start`` and ``Stop`` methods on the 
	``Application`` to tell it when to start and stop doing its thing.

Depois, adicionamos a ``MyApp Application`` para o n� origem e chamamos os m�todos ``Start`` e ``Stop`` para dizer quando e iniciar e parar a simula��o.

..
	We need to actually do the connect from the receiver point-to-point ``NetDevice``
	to our callback now.

Precisamos agora fazer a conex�o entre o receptor com nossa *callback*.

::

  devices.Get (1)->TraceConnectWithoutContext("PhyRxDrop", MakeCallback (&RxDrop));

..
	It should now be obvious that we are getting a reference to the receiving 
	``Node NetDevice`` from its container and connecting the trace source defined
	by the attribute "PhyRxDrop" on that device to the trace sink ``RxDrop``.

Estamos obtendo uma refer�ncia para o ``Node NetDevice`` receptor e conectando a origem do rastreamento pelo Atributo "PhyRxDrop" do dispositivo no destino do rastreamento ``RxDrop``.

..
	Finally, we tell the simulator to override any ``Applications`` and just
	stop processing events at 20 seconds into the simulation.

Finalmente, dizemos ao simulador para sobrescrever qualquer ``Applications`` e parar o processamento de eventos em 20 segundos na simula��o.

::

    Simulator::Stop (Seconds(20));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
  }

..
	Recall that as soon as ``Simulator::Run`` is called, configuration time
	ends, and simulation time begins.  All of the work we orchestrated by 
	creating the ``Application`` and teaching it how to connect and send
	data actually happens during this function call.

Lembre-se que quando ``Simulator::Run`` � chamado, a fase de configura��o termina e a fase de simula��o inicia. Todo o processo descrito anteriormente ocorre durante a chamada dessa fun��o.

..
	As soon as ``Simulator::Run`` returns, the simulation is complete and
	we enter the teardown phase.  In this case, ``Simulator::Destroy`` takes
	care of the gory details and we just return a success code after it completes.

Ap�s o retorno do ``Simulator::Run``, a simula��o � terminada e entramos na fase de finaliza��o. Neste caso, ``Simulator::Destroy`` executa a tarefa pesada e n�s apenas retornamos o c�digo de sucesso.

.. 
	Running fifth.cc

Executando fifth.cc
+++++++++++++++++++

..
	Since we have provided the file ``fifth.cc`` for you, if you have built
	your distribution (in debug mode since it uses NS_LOG -- recall that optimized
	builds optimize out NS_LOGs) it will be waiting for you to run.

O arquivo ``fifth.cc`` � distribu�do no c�digo fonte, no diret�rio ``examples/tutorial``. Para executar:

::

  ./waf --run fifth
  Waf: Entering directory `/home/craigdo/repos/ns-3-allinone-dev/ns-3-dev/build
  Waf: Leaving directory `/home/craigdo/repos/ns-3-allinone-dev/ns-3-dev/build'
  'build' finished successfully (0.684s)
  1.20919 1072
  1.21511 1608
  1.22103 2144
  ...
  1.2471  8040
  1.24895 8576
  1.2508  9112
  RxDrop at 1.25151
  ...

..
	You can probably see immediately a downside of using prints of any kind in your
	traces.  We get those extraneous waf messages printed all over our interesting
	information along with those RxDrop messages.  We will remedy that soon, but I'm
	sure you can't wait to see the results of all of this work.  Let's redirect that
	output to a file called ``cwnd.dat``:

Podemos observar o lado negativo de usar "prints" de qualquer tipo no rastreamento. Temos mensagens ``waf`` sendo impressas sobre a informa��o relevante. Vamos resolver esse problema, mas primeiro vamos verificar o resultado redirecionando a sa�da para um arquivo ``cwnd.dat``:

::

  ./waf --run fifth > cwnd.dat 2>&1

..
	Now edit up "cwnd.dat" in your favorite editor and remove the waf build status
	and drop lines, leaving only the traced data (you could also comment out the
	``TraceConnectWithoutContext("PhyRxDrop", MakeCallback (&RxDrop));`` in the
	script to get rid of the drop prints just as easily. 

Removemos as mensagens do ``waf`` e deixamos somente os dados rastreados.  Pode-se tamb�m comentar as mensagens de "RxDrop...".

..
	You can now run gnuplot (if you have it installed) and tell it to generate some 
	pretty pictures:

Agora podemos executar o gnuplot (se instalado) e gerar um gr�fico:

::

  gnuplot> set terminal png size 640,480
  gnuplot> set output "cwnd.png"
  gnuplot> plot "cwnd.dat" using 1:2 title 'Congestion Window' with linespoints
  gnuplot> exit

..
	You should now have a graph of the congestion window versus time sitting in the 
	file "cwnd.png" that looks like:

Devemos obter um gr�fico da janela de congestionamento pelo tempo no arquivo "cwnd.png", similar ao gr�fico 7.1:

figure:: figures/cwnd.png

   Gr�fico da janela de congestionamento versus tempo.

.. 
	Using Mid-Level Helpers

Usando Auxiliares Intermedi�rios
++++++++++++++++++++++++++++++++

..
	In the previous section, we showed how to hook a trace source and get hopefully
	interesting information out of a simulation.  Perhaps you will recall that we 
	called logging to the standard output using ``std::cout`` a "Blunt Instrument" 
	much earlier in this chapter.  We also wrote about how it was a problem having
	to parse the log output in order to isolate interesting information.  It may 
	have occurred to you that we just spent a lot of time implementing an example
	that exhibits all of the problems we purport to fix with the |ns3| tracing
	system!  You would be correct.  But, bear with us.  We're not done yet.

Na se��o anterior, mostramos como adicionar uma origem do rastreamento e obter informa��es de interesse fora da simula��o. Entretanto, no in�cio do cap�tulo foi comentado que imprimir informa��es na sa�da padr�o n�o � uma boa pr�tica. Al�m disso, comentamos que n�o � interessante realizar processamento sobre a sa�da para isolar a informa��o de interesse. Podemos pensar que perdemos muito tempo em um exemplo que apresenta todos os problemas que propomos resolver usando o sistema de rastreamento do |ns3|. Voc� estaria correto, mas n�s ainda n�o terminamos.

..
	One of the most important things we want to do is to is to have the ability to 
	easily control the amount of output coming out of the simulation; and we also 
	want to save those data to a file so we can refer back to it later.  We can use
	the mid-level trace helpers provided in |ns3| to do just that and complete
	the picture.

Uma da coisas mais importantes que queremos fazer � controlar a quantidade de sa�da da simula��o. N�s podemos usar assistentes de rastreamento intermedi�rios fornecido pelo |ns3| para alcan�ar com sucesso esse objetivo.

..
	We provide a script that writes the cwnd change and drop events developed in 
	the example ``fifth.cc`` to disk in separate files.  The cwnd changes are 
	stored as a tab-separated ASCII file and the drop events are stored in a pcap
	file.  The changes to make this happen are quite small.

Fornecemos um c�digo que separa em arquivos distintos no disco os eventos de modifica��o da janela e os eventos de remo��o. As altera��es em cwnd s�o armazenadas em um arquivo ASCII separadas por TAB e os eventos de remo��o s�o armazenados em um arquivo *pcap*. As altera��es para obter esse resultado s�o pequenas.

.. 
	A sixth.cc Walkthrough

Analisando sixth.cc
~~~~~~~~~~~~~~~~~~~

..
	Let's take a look at the changes required to go from ``fifth.cc`` to 
	``sixth.cc``.  Open ``examples/tutorial/fifth.cc`` in your favorite 
	editor.  You can see the first change by searching for CwndChange.  You will 
	find that we have changed the signatures for the trace sinks and have added 
	a single line to each sink that writes the traced information to a stream
	representing a file.

Vamos verificar as mudan�as do arquivo ``fifth.cc`` para o  ``sixth.cc``. Verificamos a primeira mudan�a em ``CwndChange``. Notamos que as assinaturas para o destino do rastreamento foram alteradas e que foi adicionada uma linha para cada um que escreve a informa��o rastreada para um fluxo (*stream*) representando um arquivo

::

  static void
  CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
  {
    NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
    *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" 
    		<< oldCwnd << "\t" << newCwnd << std::endl;
  }
  
  static void
  RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
  {
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
    file->Write(Simulator::Now(), p);
  }

..
	We have added a "stream" parameter to the ``CwndChange`` trace sink.  
	This is an object that holds (keeps safely alive) a C++ output stream.  It 
	turns out that this is a very simple object, but one that manages lifetime 
	issues for the stream and solves a problem that even experienced C++ users 
	run into.  It turns out that the copy constructor for ostream is marked 
	private.  This means that ostreams do not obey value semantics and cannot 
	be used in any mechanism that requires the stream to be copied.  This includes
	the |ns3| callback system, which as you may recall, requires objects
	that obey value semantics.  Further notice that we have added the following 
	line in the ``CwndChange`` trace sink implementation:

Um par�metro "stream" foi adicionado para o destino do rastreamento ``CwndChange``. Este � um objeto que armazena (mant�m seguramente vivo) um fluxo de sa�da em C++. Isto resulta em um objeto muito simples, mas que ger�ncia problemas no ciclo de vida para fluxos e resolve um problema que mesmo programadores experientes de C++ tem dificuldades. Resulta que o construtor de c�pia para o fluxo de sa�da (*ostream*) � marcado como privado. Isto significa que fluxos de sa�da n�o seguem a sem�ntica de passagem por valor e n�o podem ser usados em mecanismos que necessitam que o fluxo seja copiado. Isto inclui o sistema de *callback* do |ns3|. Al�m disso, adicionamos a seguinte linha:

::

  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd 
  		<< "\t" << newCwnd << std::endl;

..
	This would be very familiar code if you replaced ``*stream->GetStream ()``
	with ``std::cout``, as in:

que substitui ``std::cout`` por ``*stream->GetStream ()``

::

  std::cout << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << 
  		newCwnd << std::endl;

..
	This illustrates that the ``Ptr<OutputStreamWrapper>`` is really just
	carrying around a ``std::ofstream`` for you, and you can use it here like 
	any other output stream.

Isto demostra que o ``Ptr<OutputStreamWrapper>`` est� apenas encapsulando um ``std::ofstream``, logo pode ser usado como qualquer outro fluxo de sa�da.

..
	A similar situation happens in ``RxDrop`` except that the object being 
	passed around (a ``Ptr<PcapFileWrapper>``) represents a pcap file.  There
	is a one-liner in the trace sink to write a timestamp and the contents of the 
	packet being dropped to the pcap file:

Uma situa��o similar ocorre em ``RxDrop``, exceto que o objeto passado (``Ptr<PcapFileWrapper>``) representa um arquivo pcap. H� uma linha no *trace sink* para escrever um marcador de tempo (*timestamp*) eo conte�do do pacote perdido para o arquivo pcap.

::

  file->Write(Simulator::Now(), p);

..
	Of course, if we have objects representing the two files, we need to create
	them somewhere and also cause them to be passed to the trace sinks.  If you 
	look in the ``main`` function, you will find new code to do just that:

� claro, se n�s temos objetos representando os dois arquivos, precisamos cri�-los em algum lugar e tamb�m pass�-los aos *trace sinks*. Se observarmos a fun��o ``main``, temos o c�digo:

::

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("sixth.cwnd");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", 
  		MakeBoundCallback (&CwndChange, stream));

  ...

  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("sixth.pcap", 
  		std::ios::out, PcapHelper::DLT_PPP);
  devices.Get (1)->TraceConnectWithoutContext("PhyRxDrop", 
  		MakeBoundCallback (&RxDrop, file));

..
	In the first section of the code snippet above, we are creating the ASCII
	trace file, creating an object responsible for managing it and using a
	variant of the callback creation function to arrange for the object to be 
	passed to the sink.  Our ASCII trace helpers provide a rich set of
	functions to make using text (ASCII) files easy.  We are just going to 
	illustrate the use of the file stream creation function here.

Na primeira se��o do c�digo, criamos o arquivo de rastreamento ASCII e o objeto respons�vel para gerenci�-lo. Em seguida, usando uma das formas da fun��o para cria��o da *callback* permitimos o objeto ser passado para o destino do rastreamento. As classes assistentes para rastreamento ASCII fornecem um vasto conjunto de fun��es para facilitar a manipula��o de arquivos texto. Neste exemplo, focamos apenas na cria��o do arquivo para o fluxo de sa�da.

..
	The ``CreateFileStream{}`` function is basically going to instantiate
	a std::ofstream object and create a new file (or truncate an existing file).
	This ofstream is packaged up in an |ns3| object for lifetime management
	and copy constructor issue resolution.

A fun��o ``CreateFileStream()`` instancia um objeto ``std::ofstream`` e cria  um novo arquivo. O fluxo de sa�da ``ofstream`` � encapsulado em um objeto do |ns3| para gerenciamento do ciclo de vida e para resolver o
problema do construtor de c�pia.

..
	We then take this |ns3| object representing the file and pass it to
	``MakeBoundCallback()``.  This function creates a callback just like
	``MakeCallback()``, but it "binds" a new value to the callback.  This
	value is added to the callback before it is called.

Ent�o pegamos o objeto que representa o arquivo e passamos para ``MakeBoundCallback()``. Esta fun��o cria um *callback* como ``MakeCallback()``, mas "associa" um novo valor para o *callback*. Este valor � adicionado ao *callback* antes de sua invoca��o.

..
	Essentially, ``MakeBoundCallback(&CwndChange, stream)`` causes the trace 
	source to add the additional "stream" parameter to the front of the formal
	parameter list before invoking the callback.  This changes the required 
	signature of the ``CwndChange`` sink to match the one shown above, which
	includes the "extra" parameter ``Ptr<OutputStreamWrapper> stream``.

Essencialmente, ``MakeBoundCallback(&CwndChange, stream)`` faz com que a origem do rastreamento adicione um par�metro extra "fluxo" ap�s a lista formal de par�metros antes de invocar o *callback*. Esta mudan�a est� de acordo com o apresentado anteriormente, a qual inclui o par�metro ``Ptr<OutputStreamWrapper> stream``.

..
	In the second section of code in the snippet above, we instantiate a 
	``PcapHelper`` to do the same thing for our pcap trace file that we did
	with the ``AsciiTraceHelper``. The line of code,

Na segunda se��o de c�digo, instanciamos um ``PcapHelper`` para fazer a mesma coisa para o arquivo de rastreamento pcap. A linha de c�digo,

::

  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("sixth.pcap", "w", 
  		PcapHelper::DLT_PPP);

..
	creates a pcap file named "sixth.pcap" with file mode "w".   This means that
	the new file is to truncated if an existing file with that name is found.  The
	final parameter is the "data link type" of the new pcap file.  These are 
	the same as the pcap library data link types defined in ``bpf.h`` if you are
	familar with pcap.  In this case, ``DLT_PPP`` indicates that the pcap file
	is going to contain packets prefixed with point to point headers.  This is true
	since the packets are coming from our point-to-point device driver.  Other
	common data link types are DLT_EN10MB (10 MB Ethernet) appropriate for csma
	devices and DLT_IEEE802_11 (IEEE 802.11) appropriate for wifi devices.  These
	are defined in ``src/network/helper/trace-helper.h"`` if you are interested in seeing
	the list.  The entries in the list match those in ``bpf.h`` but we duplicate
	them to avoid a pcap source dependence.

cria um arquivo pcap chamado "sixth.pcap" no modo "w" (escrita). O par�metro final � o "tipo da liga��o de dados" do arquivo pcap. As op��es est�o definidas em ``bpf.h``. Neste caso, ``DLT_PPP`` indica que o arquivo pcap dever� conter pacotes prefixado com cabe�alhos ponto-a-ponto. Isto � verdade pois os pacotes est�o chegando de nosso `driver` de dispositivo ponto-a-ponto. Outros tipos de liga��o de dados comuns s�o DLT_EN10MB (10 MB Ethernet) apropriado para dispositivos CSMA e DLT_IEEE802_11 (IEEE 802.11) apropriado para dispositivos sem fio. O arquivo ``src/network/helper/trace-helper.h"`` define uma lista com os tipos. As entradas na lista s�o id�nticas as definidas em ``bpf.h``, pois foram duplicadas para evitar um depend�ncia com o pcap.

..
	A |ns3| object representing the pcap file is returned from ``CreateFile``
	and used in a bound callback exactly as it was in the ascii case.

Um objeto |ns3| representando o arquivo pcap � retornado de ``CreateFile`` e usado em uma *callback* exatamente como no caso ASCII.

..
	An important detour:  It is important to notice that even though both of these 
	objects are declared in very similar ways,

� importante observar que ambos objetos s�o declarados de maneiras muito similares,

::

  Ptr<PcapFileWrapper> file ...
  Ptr<OutputStreamWrapper> stream ...

..
	The underlying objects are entirely different.  For example, the 
	Ptr<PcapFileWrapper> is a smart pointer to an |ns3| Object that is a 
	fairly heaviweight thing that supports ``Attributes`` and is integrated into
	the config system.  The Ptr<OutputStreamWrapper>, on the other hand, is a smart 
	pointer to a reference counted object that is a very lightweight thing.
	Remember to always look at the object you are referencing before making any
	assumptions about the "powers" that object may have.  

Mas os objetos internos s�o inteiramente diferentes. Por exemplo, o Ptr<PcapFileWrapper> � um ponteiro para um objeto |ns3| que suporta ``Attributes`` e � integrado dentro do sistema de configura��o. O Ptr<OutputStreamWrapper>, por outro lado, � um ponteiro para uma refer�ncia para um simples objeto contado. Lembre-se sempre de analisar o objeto que voc� est� referenciando antes de fazer suposi��es sobre os "poderes" que o objeto pode ter.

..
	For example, take a look at ``src/network/utils/pcap-file-wrapper.h`` in the 
	distribution and notice, 

Por exemplo, acesse o arquivo ``src/network/utils/pcap-file-wrapper.h`` e observe,

::

  class PcapFileWrapper : public Object

..
	that class ``PcapFileWrapper`` is an |ns3| Object by virtue of 
	its inheritance.  Then look at ``src/network/model/output-stream-wrapper.h`` and 
	notice,

que a classe ``PcapFileWrapper`` � um ``Object`` |ns3| por heran�a. J� no arquivo ``src/network/model/output-stream-wrapper.h``, observe,

::

  class OutputStreamWrapper : public SimpleRefCount<OutputStreamWrapper>

..
	that this object is not an |ns3| Object at all, it is "merely" a
	C++ object that happens to support intrusive reference counting.

que n�o � um ``Object`` |ns3|, mas um objeto C++ que suporta contagem de refer�ncia.

..
	The point here is that just because you read Ptr<something> it does not necessarily
	mean that "something" is an |ns3| Object on which you can hang |ns3|
	``Attributes``, for example.

A quest�o � que se voc� tem um Ptr<alguma_coisa>, n�o necessariamente significa que "alguma_coisa" � um ``Object`` |ns3|, no qual voc� pode modificar ``Attributes``, por exemplo.

..
	Now, back to the example.  If you now build and run this example,

Voltando ao exemplo. Se compilarmos e executarmos o exemplo,

::

  ./waf --run sixth

..
	you will see the same messages appear as when you ran "fifth", but two new 
	files will appear in the top-level directory of your |ns3| distribution.

Veremos as mesmas mensagens do "fifth", mas dois novos arquivos aparecer�o no diret�rio base de sua distribui��o do |ns3|.

::

  sixth.cwnd  sixth.pcap

..
	Since "sixth.cwnd" is an ASCII text file, you can view it with ``cat``
	or your favorite file viewer.

Como "sixth.cwnd" � um arquivo texto ASCII, voc� pode visualizar usando *cat* ou um editor de texto.

::

  1.20919 536     1072
  1.21511 1072    1608
  ...
  9.30922 8893    8925
  9.31754 8925    8957

..
	You have a tab separated file with a timestamp, an old congestion window and a
	new congestion window suitable for directly importing into your plot program.
	There are no extraneous prints in the file, no parsing or editing is required.
	
Cada linha tem um marcador de tempo, o valor da janela de congestionamento e o valor da nova janela de congestionamento separados por tabula��o, para importar diretamente para seu programa de plotagem de gr�ficos.
N�o h� nenhuma outra informa��o al�m da rastreada, logo n�o � necess�rio processamento ou edi��o do arquivo.

..
	Since "sixth.pcap" is a pcap file, you can view it with ``tcpdump``.

Como "sixth.pcap" � um arquivo pcap, voc� pode visualizar usando o ``tcpdump`` ou ``wireshark``.

::

  reading from file ../../sixth.pcap, link-type PPP (PPP)
  1.251507 IP 10.1.1.1.49153 > 10.1.1.2.8080: . 17689:18225(536) ack 1 win 65535
  1.411478 IP 10.1.1.1.49153 > 10.1.1.2.8080: . 33808:34312(504) ack 1 win 65535
  ...
  7.393557 IP 10.1.1.1.49153 > 10.1.1.2.8080: . 781568:782072(504) ack 1 win 65535
  8.141483 IP 10.1.1.1.49153 > 10.1.1.2.8080: . 874632:875168(536) ack 1 win 65535

..
	You have a pcap file with the packets that were dropped in the simulation.  There
	are no other packets present in the file and there is nothing else present to
	make life difficult.

Voc� tem um arquivo pcap com os pacotes que foram descartados na simula��o. N�o h� nenhum outro pacote presente no arquivo e nada mais para dificultar sua an�lise.

..
	It's been a long journey, but we are now at a point where we can appreciate the
	|ns3| tracing system.  We have pulled important events out of the middle
	of a TCP implementation and a device driver.  We stored those events directly in
	files usable with commonly known tools.  We did this without modifying any of the
	core code involved, and we did this in only 18 lines of code:

Foi uma longa jornada, mas agora entendemos porque o sistema de rastreamento � interessante. N�s obtemos e armazenamos importantes eventos da implementa��o do TCP e do `driver` de dispositivo. E n�o modificamos nenhuma linha do c�digo do n�cleo do |ns3|, e ainda fizemos isso com apenas 18 linhas de c�digo:

::

  static void
  CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
  {
    NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
    *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << 
    		oldCwnd << "\t" << newCwnd << std::endl;
  }

  ...

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("sixth.cwnd");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", 
  		MakeBoundCallback (&CwndChange, stream));

  ...

  static void
  RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
  {
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
    file->Write(Simulator::Now(), p);
  }

  ...
  
  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("sixth.pcap", "w", 
  		PcapHelper::DLT_PPP);
  devices.Get (1)->TraceConnectWithoutContext("PhyRxDrop", 
  		MakeBoundCallback (&RxDrop, file));

.. 
	Using Trace Helpers

Usando Classes Assistentes para Rastreamento
********************************************

..
	The |ns3| trace helpers provide a rich environment for configuring and
	selecting different trace events and writing them to files.  In previous
	sections, primarily "Building Topologies," we have seen several varieties
	of the trace helper methods designed for use inside other (device) helpers.

As classes assistentes (*trace helpers*) de rastreamento do |ns3| proveem um ambiente rico para configurar, selecionar e escrever diferentes eventos de rastreamento para arquivos. Nas se��es anteriores, primeiramente em "Construindo Topologias", n�s vimos diversas formas de m�todos assistentes para rastreamento projetados para uso dentro de outras classes assistentes.

..
	Perhaps you will recall seeing some of these variations: 

Segue alguns desses m�todos j� estudados:

::

  pointToPoint.EnablePcapAll ("second");
  pointToPoint.EnablePcap ("second", p2pNodes.Get (0)->GetId (), 0);
  csma.EnablePcap ("third", csmaDevices.Get (0), true);
  pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("myfirst.tr"));

..
	What may not be obvious, though, is that there is a consistent model for all of 
	the trace-related methods found in the system.  We will now take a little time
	and take a look at the "big picture".


O que n�o parece claro � que h� um modelo consistente para todos os m�todos relacionados � rastreamento encontrados no sistema. Apresentaremos uma vis�o geral desse modelo.

..
	There are currently two primary use cases of the tracing helpers in |ns3|:
	Device helpers and protocol helpers.  Device helpers look at the problem
	of specifying which traces should be enabled through a node, device pair.  For 
	example, you may want to specify that pcap tracing should be enabled on a 
	particular device on a specific node.  This follows from the |ns3| device
	conceptual model, and also the conceptual models of the various device helpers.
	Following naturally from this, the files created follow a 
	<prefix>-<node>-<device> naming convention.  

H� dois casos de uso prim�rios de classes assistentes em |ns3|: Classes assistentes de dispositivo e classes assistentes de protocolo. Classes assistentes de dispositivo tratam o problema de especificar quais rastreamentos deveriam ser habilitados no dom�nio do n� de rede. Por exemplo, poder�amos querer especificar que o rastreamento pcap deveria ser ativado em um dispositivo particular de um n� espec�fico. Isto � o que define o modelo conceitual de dispositivo no |ns3| e tamb�m os modelos conceituais de v�rias classes assistentes de dispositivos. Baseado nisso, os arquivos criados seguem a conven��o de nome `<prefixo>-<n�>-<dispositivo>`.

..
	Protocol helpers look at the problem of specifying which traces should be
	enabled through a protocol and interface pair.  This follows from the |ns3|
	protocol stack conceptual model, and also the conceptual models of internet
	stack helpers.  Naturally, the trace files should follow a 
	<prefix>-<protocol>-<interface> naming convention.

As classes assistentes de protocolos tratam o problema de especificar quais rastreamentos deveriam ser ativados no protocolo e interface. Isto � definido pelo modelo conceitual de pilha de protocolo do |ns3| e tamb�m pelos modelos conceituais de classes assistentes de pilha de rede. Baseado nisso, os arquivos criados seguem a conven��o de nome `<prefixo>-<protocolo>-<interface>`.

..
	The trace helpers therefore fall naturally into a two-dimensional taxonomy.
	There are subtleties that prevent all four classes from behaving identically,
	but we do strive to make them all work as similarly as possible; and whenever
	possible there are analogs for all methods in all classes.

As classes assistentes consequentemente encaixam-se em uma taxinomia bi-dimensional. H� pequenos detalhes que evitam todas as classes comportarem-se da mesma forma, mas fizemos parecer que trabalham t�o similarmente quanto poss�vel e quase sempre h� similares para todos m�todos em todas as classes.

::

                                                     | pcap | ascii |
  ---------------------------------------------------+------+-------|
  Classe Assistente de Dispositivo (*Device Helper*)   |      |       |
  ---------------------------------------------------+------+-------|
  Classe Assistente de Protocolo (*Protocol Helper*)   |      |       |
  ---------------------------------------------------+------+-------|

..
	We use an approach called a ``mixin`` to add tracing functionality to our 
	helper classes.  A ``mixin`` is a class that provides functionality to that
	is inherited by a subclass.  Inheriting from a mixin is not considered a form 
	of specialization but is really a way to collect functionality. 

Usamos uma abordagem chamada ``mixin`` para adicionar funcionalidade de rastreamento para nossas classes assistentes. Uma ``mixin`` � uma classe que prov� funcionalidade para aquela que � herdada por uma subclasse. Herdar de um ``mixin`` n�o � considerado uma forma de especializa��o mas � realmente uma maneira de colecionar funcionalidade.

..
	Let's take a quick look at all four of these cases and their respective 
	``mixins``.

Vamos verificar rapidamente os quatro casos e seus respectivos ``mixins``.

.. 
	Pcap Tracing Device Helpers

Classes Assistentes de Dispositivo para Rastreamento Pcap
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

..
	The goal of these helpers is to make it easy to add a consistent pcap trace
	facility to an |ns3| device.  We want all of the various flavors of
	pcap tracing to work the same across all devices, so the methods of these 
	helpers are inherited by device helpers.  Take a look at 
	``src/network/helper/trace-helper.h`` if you want to follow the discussion while 
	looking at real code.

O objetivo destes assistentes � simplificar a adi��o de um utilit�rio de rastreamento pcap consistente para um dispositivo |ns3|. Queremos que opere da mesma forma entre todos os dispositivos, logo os m�todos destes assistentes s�o herdados por classes assistentes de dispositivo. Observe o arquivo ``src/network/helper/trace-helper.h`` para entender a discuss�o do c�digo a seguir.

..
	The class ``PcapHelperForDevice`` is a ``mixin`` provides the high level 
	functionality for using pcap tracing in an |ns3| device.  Every device 
	must implement a single virtual method inherited from this class.

A classe ``PcapHelperForDevice`` � um ``mixin`` que prov� a funcionalidade de alto n�vel para usar rastreamento pcap em um dispositivo |ns3|. Todo dispositivo deve implementar um �nico m�todo virtual herdado dessa classe.

::

  virtual void EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, 
  		bool promiscuous, bool explicitFilename) = 0;

..
	The signature of this method reflects the device-centric view of the situation
	at this level.  All of the public methods inherited from class 
	``PcapUserHelperForDevice`` reduce to calling this single device-dependent
	implementation method.  For example, the lowest level pcap method,

A assinatura deste m�todo reflete a vis�o do dispositivo da situa��o neste n�vel. Todos os m�todos p�blicos herdados da classe ``PcapUserHelperForDevice`` s�o reduzidos a chamada da implementa��o deste simples m�todo dependente de dispositivo. Por exemplo, o n�vel mais baixo do m�todo pcap,

::

  void EnablePcap (std::string prefix, Ptr<NetDevice> nd, bool promiscuous = false, 
  		bool explicitFilename = false);

..
	will call the device implementation of ``EnablePcapInternal`` directly.  All
	other public pcap tracing methods build on this implementation to provide 
	additional user-level functionality.  What this means to the user is that all 
	device helpers in the system will have all of the pcap trace methods available;
	and these methods will all work in the same way across devices if the device 
	implements ``EnablePcapInternal`` correctly.

chamaremos diretamente a implementa��o do dispositivo de ``EnablePcapInternal``. Todos os outros m�todos de rastreamento pcap p�blicos desta implementa��o s�o para prover funcionalidade adicional em n�vel de usu�rio. Para o usu�rio, isto significa que todas as classes assistentes de dispositivo no sistema ter�o todos os m�todos de rastreamento pcap dispon�veis; e estes m�todos trabalhar�o da mesma forma entre dispositivos se o dispositivo implementar corretamente ``EnablePcapInternal``.

.. 
	Pcap Tracing Device Helper Methods

M�todos da Classe Assistente de Dispositivo para Rastreamento Pcap
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

  void EnablePcap (std::string prefix, Ptr<NetDevice> nd, 
  		bool promiscuous = false, bool explicitFilename = false);
  void EnablePcap (std::string prefix, std::string ndName, 
  		bool promiscuous = false, bool explicitFilename = false);
  void EnablePcap (std::string prefix, NetDeviceContainer d, 
  		bool promiscuous = false);
  void EnablePcap (std::string prefix, NodeContainer n, 
  		bool promiscuous = false);
  void EnablePcap (std::string prefix, uint32_t nodeid, uint32_t deviceid, 
  		bool promiscuous = false);
  void EnablePcapAll (std::string prefix, bool promiscuous = false);

..
	In each of the methods shown above, there is a default parameter called 
	``promiscuous`` that defaults to false.  This parameter indicates that the
	trace should not be gathered in promiscuous mode.  If you do want your traces
	to include all traffic seen by the device (and if the device supports a 
	promiscuous mode) simply add a true parameter to any of the calls above.  For example,

Em cada m�todo apresentado existe um par�metro padr�o chamado ``promiscuous`` que � definido para o valor "false". Este par�metro indica que o rastreamento n�o deveria coletar dados em modo prom�scuo. Se quisermos incluir todo tr�fego visto pelo dispositivo devemos modificar o valor para "true". Por exemplo,

::

  Ptr<NetDevice> nd;
  ...
  helper.EnablePcap ("prefix", nd, true);

..
	will enable promiscuous mode captures on the ``NetDevice`` specified by ``nd``.

ativar� o modo de captura prom�scuo no ``NetDevice`` especificado por ``nd``.

..
	The first two methods also include a default parameter called ``explicitFilename``
	that will be discussed below.

Os  dois primeiros m�todos tamb�m incluem um par�metro padr�o chamado ``explicitFilename`` que ser� abordado a seguir.

..
	You are encouraged to peruse the Doxygen for class ``PcapHelperForDevice``
	to find the details of these methods; but to summarize ...

� interessante procurar maiores detalhes dos m�todos da classe ``PcapHelperForDevice`` no Doxygen; mas para resumir ...

..
	You can enable pcap tracing on a particular node/net-device pair by providing a
	``Ptr<NetDevice>`` to an ``EnablePcap`` method.  The ``Ptr<Node>`` is 
	implicit since the net device must belong to exactly one ``Node``.
	For example, 

Podemos ativar o rastreamento pcap em um par n�/dispositivo-rede espec�fico provendo um ``Ptr<NetDevice>`` para um m�todo ``EnablePcap``. O ``Ptr<Node>`` � impl�cito, pois o dispositivo de rede deve estar em um ``Node``. Por exemplo,

::

  Ptr<NetDevice> nd;
  ...
  helper.EnablePcap ("prefix", nd);

..
	You can enable pcap tracing on a particular node/net-device pair by providing a
	``std::string`` representing an object name service string to an 
	``EnablePcap`` method.  The ``Ptr<NetDevice>`` is looked up from the name
	string.  Again, the ``<Node>`` is implicit since the named net device must 
	belong to exactly one ``Node``.  For example, 

Podemos ativar o rastreamento pcap em um par n�/dispositivo-rede passando uma ``std::string`` que representa um nome de servi�o para um m�todo ``EnablePcap``. O ``Ptr<NetDevice>`` � buscado a partir do nome da `string`.
Novamente, o ``Ptr<Node>`` � impl�cito pois o dispositivo de rede deve estar em um ``Node``. 

::

  Names::Add ("server" ...);
  Names::Add ("server/eth0" ...);
  ...
  helper.EnablePcap ("prefix", "server/eth0");

..
	You can enable pcap tracing on a collection of node/net-device pairs by 
	providing a ``NetDeviceContainer``.  For each ``NetDevice`` in the container
	the type is checked.  For each device of the proper type (the same type as is 
	managed by the device helper), tracing is enabled.    Again, the ``<Node>`` is 
	implicit since the found net device must belong to exactly one ``Node``.
	For example, 

Podemos ativar o rastreamento pcap em uma cole��o de pares n�s/dispositivos usando um ``NetDeviceContainer``. Para cada ``NetDevice`` no cont�iner o tipo � verificado. Para cada dispositivo com o tipo adequado, o rastreamento ser� ativado. Por exemplo,

::

  NetDeviceContainer d = ...;
  ...
  helper.EnablePcap ("prefix", d);

..
	You can enable pcap tracing on a collection of node/net-device pairs by 
	providing a ``NodeContainer``.  For each ``Node`` in the ``NodeContainer``
	its attached ``NetDevices`` are iterated.  For each ``NetDevice`` attached
	to each node in the container, the type of that device is checked.  For each 
	device of the proper type (the same type as is managed by the device helper), 
	tracing is enabled.

Podemos ativar o rastreamento em uma cole��o de pares n�/dispositivo-rede usando um ``NodeContainer``. Para cada ``Node`` no ``NodeContainer`` seus ``NetDevices`` s�o percorridos e verificados segundo o tipo. Para cada dispositivo com o tipo adequado, o rastreamento � ativado.

::

  NodeContainer n;
  ...
  helper.EnablePcap ("prefix", n);

..
	You can enable pcap tracing on the basis of node ID and device ID as well as
	with explicit ``Ptr``.  Each ``Node`` in the system has an integer node ID
	and each device connected to a node has an integer device ID.

Podemos ativar o rastreamento pcap usando o n�mero identificador (`ID`) do n� e do dispositivo. Todo ``Node`` no sistema tem um valor inteiro indicando o `ID` do n� e todo dispositivo conectado ao n� tem um valor inteiro indicando o `ID` do dispositivo.

::

  helper.EnablePcap ("prefix", 21, 1);

..
	Finally, you can enable pcap tracing for all devices in the system, with the
	same type as that managed by the device helper.

Por fim, podemos ativar rastreamento pcap para todos os dispositivos no sistema, desde que o tipo seja o mesmo gerenciado pela classe assistentes de dispositivo.

::

  helper.EnablePcapAll ("prefix");

.. 
	Pcap Tracing Device Helper Filename Selection

Sele��o de um Nome de Arquivo para o Rastreamento Pcap da Classe Assistente de Dispositivo
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

..
	Implicit in the method descriptions above is the construction of a complete 
	filename by the implementation method.  By convention, pcap traces in the 
	|ns3| system are of the form "<prefix>-<node id>-<device id>.pcap"

Impl�cito nas descri��es de m�todos anteriores � a constru��o do nome de arquivo por meio do m�todo da implementa��o. Por conven��o, rastreamento pcap no |ns3| usa a forma "<prefixo>-<id do n�>-<id do dispositivo>.pcap"

..
	As previously mentioned, every node in the system will have a system-assigned
	node id; and every device will have an interface index (also called a device id)
	relative to its node.  By default, then, a pcap trace file created as a result
	of enabling tracing on the first device of node 21 using the prefix "prefix"
	would be "prefix-21-1.pcap".
	
Como mencionado, todo n� no sistema ter� um `id` de n� associado; e todo dispositivo ter� um �ndice de interface (tamb�m chamado de id do dispositivo) relativo ao seu n�. Por padr�o, ent�o, um arquivo pcap criado como um resultado de ativar rastreamento no primeiro dispositivo do n� 21 usando o prefixo "prefix" seria "prefix-21-1.pcap".

..
	You can always use the |ns3| object name service to make this more clear.
	For example, if you use the object name service to assign the name "server"
	to node 21, the resulting pcap trace file name will automatically become,
	"prefix-server-1.pcap" and if you also assign the name "eth0" to the 
	device, your pcap file name will automatically pick this up and be called
	"prefix-server-eth0.pcap".

Sempre podemos usar o servi�o de nome de objeto do |ns3| para tornar isso mais claro. Por exemplo, se voc� usa o servi�o para associar o nome "server" ao n� 21, o arquivo pcap resultante automaticamente ser�, "prefix-server-1.pcap" e se voc� tamb�m associar o nome "eth0" ao dispositivo, seu nome do arquivo pcap automaticamente ser� denominado "prefix-server-eth0.pcap".

.. 
	Finally, two of the methods shown above,

Finalmente, dois dos m�todos mostrados, 

::

  void EnablePcap (std::string prefix, Ptr<NetDevice> nd, 
  		bool promiscuous = false, bool explicitFilename = false);
  void EnablePcap (std::string prefix, std::string ndName, 
  		bool promiscuous = false, bool explicitFilename = false);

..
	have a default parameter called ``explicitFilename``.  When set to true,
	this parameter disables the automatic filename completion mechanism and allows
	you to create an explicit filename.  This option is only available in the 
	methods which enable pcap tracing on a single device.  

tem um par�metro padr�o ``explicitFilename``. Quando modificado para verdadeiro, este par�metro desabilita o mecanismo autom�tico de completar o nome do arquivo e permite criarmos um nome de arquivo abertamente. Esta op��o est� dispon�vel nos m�todos que ativam o rastreamento pcap em um �nico dispositivo.

..
	For example, in order to arrange for a device helper to create a single 
	promiscuous pcap capture file of a specific name ("my-pcap-file.pcap") on a
	given device, one could:

Por exemplo, com a finalidade providenciar uma classe assistente de dispositivo para criar um �nico arquivo de captura pcap no modo prom�scuo com um nome espec�fico ("my-pcap-file.pcap") em um determinado dispositivo:
	
::

  Ptr<NetDevice> nd;
  ...
  helper.EnablePcap ("my-pcap-file.pcap", nd, true, true);

..
	The first ``true`` parameter enables promiscuous mode traces and the second
	tells the helper to interpret the ``prefix`` parameter as a complete filename.

O primeiro par�metro ``true`` habilita o modo de rastreamento prom�scuo e o segundo faz com que o par�metro ``prefix`` seja interpretado como um nome de arquivo completo.

.. 
	Ascii Tracing Device Helpers

Classes Assistentes de Dispositivo para Rastreamento ASCII
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

..
	The behavior of the ascii trace helper ``mixin`` is substantially similar to 
	the pcap version.  Take a look at ``src/network/helper/trace-helper.h`` if you want to 
	follow the discussion while looking at real code.

O comportamento do assistente  de rastreamento ASCII ``mixin`` � similar a vers�o do pcap. Acesse o arquivo ``src/network/helper/trace-helper.h`` para compreender melhor o funcionamento dessa classe assistente.

..
	The class ``AsciiTraceHelperForDevice`` adds the high level functionality for 
	using ascii tracing to a device helper class.  As in the pcap case, every device
	must implement a single virtual method inherited from the ascii trace ``mixin``.

A classe ``AsciiTraceHelperForDevice`` adiciona funcionalidade em alto n�vel para usar o rastreamento ASCII para uma classe assistente de dispositivo. Como no caso do pcap, todo dispositivo deve implementar um m�todo herdado do rastreador ASCII ``mixin``.

::

  virtual void EnableAsciiInternal (Ptr<OutputStreamWrapper> stream,
		std::string prefix, Ptr<NetDevice> nd, bool explicitFilename) = 0;

..
	The signature of this method reflects the device-centric view of the situation
	at this level; and also the fact that the helper may be writing to a shared
	output stream.  All of the public ascii-trace-related methods inherited from 
	class ``AsciiTraceHelperForDevice`` reduce to calling this single device-
	dependent implementation method.  For example, the lowest level ascii trace
	methods,

A assinatura deste m�todo reflete a vis�o do dispositivo da situa��o neste n�vel; e tamb�m o fato que o assistente pode ser escrito para um fluxo de sa�da compartilhado. Todos os m�todos p�blicos associados ao rastreamento ASCII herdam da classe ``AsciiTraceHelperForDevice`` resumem-se a chamada deste �nico m�todo dependente de implementa��o. Por exemplo, os m�todos de rastreamento ASCII de mais baixo n�vel,

::

  void EnableAscii (std::string prefix, Ptr<NetDevice> nd, 
  		bool explicitFilename = false);
  void EnableAscii (Ptr<OutputStreamWrapper> stream, Ptr<NetDevice> nd);

.. 
	will call the device implementation of ``EnableAsciiInternal`` directly,
	providing either a valid prefix or stream.  All other public ascii tracing 
	methods will build on these low-level functions to provide additional user-level
	functionality.  What this means to the user is that all device helpers in the 
	system will have all of the ascii trace methods available; and these methods
	will all work in the same way across devices if the devices implement 
	``EnablAsciiInternal`` correctly.

chamar�o uma implementa��o de ``EnableAsciiInternal`` diretamente, passando um prefixo ou fluxo v�lido. Todos os outros m�todos p�blicos ser�o constru�dos a partir destas fun��es de baixo n�vel para fornecer funcionalidades adicionais em n�vel de usu�rio. Para o usu�rio, isso significa que todos os assistentes de
dispositivo no sistema ter�o todos os m�todos de rastreamento ASCII dispon�veis e estes m�todos trabalhar�o do mesmo modo em todos os dispositivos se estes implementarem ``EnableAsciiInternal``.

.. 
	Ascii Tracing Device Helper Methods

M�todos da Classe Assistente de Dispositivo para Rastreamento  ASCII
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

  void EnableAscii (std::string prefix, Ptr<NetDevice> nd, 
  		bool explicitFilename = false);
  void EnableAscii (Ptr<OutputStreamWrapper> stream, Ptr<NetDevice> nd);

  void EnableAscii (std::string prefix, std::string ndName, 
  		bool explicitFilename = false);
  void EnableAscii (Ptr<OutputStreamWrapper> stream, std::string ndName);

  void EnableAscii (std::string prefix, NetDeviceContainer d);
  void EnableAscii (Ptr<OutputStreamWrapper> stream, NetDeviceContainer d);

  void EnableAscii (std::string prefix, NodeContainer n);
  void EnableAscii (Ptr<OutputStreamWrapper> stream, NodeContainer n);

  void EnableAsciiAll (std::string prefix);
  void EnableAsciiAll (Ptr<OutputStreamWrapper> stream);

  void EnableAscii (std::string prefix, uint32_t nodeid, uint32_t deviceid, 
  		bool explicitFilename);
  void EnableAscii (Ptr<OutputStreamWrapper> stream, uint32_t nodeid, 
  		uint32_t deviceid);

..
	You are encouraged to peruse the Doxygen for class ``AsciiTraceHelperForDevice``
	to find the details of these methods; but to summarize ...

Para maiores detalhes sobre os m�todos � interessante consultar a documenta��o para a classe ``AsciiTraceHelperForDevice``; mas para resumir ...

..
	There are twice as many methods available for ascii tracing as there were for
	pcap tracing.  This is because, in addition to the pcap-style model where traces
	from each unique node/device pair are written to a unique file, we support a model
	in which trace information for many node/device pairs is written to a common file.
	This means that the <prefix>-<node>-<device> file name generation mechanism is 
	replaced by a mechanism to refer to a common file; and the number of API methods
	is doubled to allow all combinations.

H� duas vezes mais m�todos dispon�veis para rastreamento ASCII que para rastreamento pcap. Isto ocorre pois para o modelo pcap os rastreamentos de cada par n�/dispositivo-rede s�o escritos para um �nico arquivo, enquanto que no ASCII todo as as informa��es s�o escritas para um arquivo comum. Isto significa que o mecanismo de gera��o de nomes de arquivos `<prefixo>-<n�>-<dispositivo>` � substitu�do por um mecanismo para referenciar um arquivo comum; e o n�mero de m�todos da API � duplicado para permitir todas as combina��es.

..
	Just as in pcap tracing, you can enable ascii tracing on a particular 
	node/net-device pair by providing a ``Ptr<NetDevice>`` to an ``EnableAscii``
	method.  The ``Ptr<Node>`` is implicit since the net device must belong to 
	exactly one ``Node``.  For example, 

Assim como no rastreamento pcap, podemos ativar o rastreamento ASCII em um par n�/dispositivo-rede passando um ``Ptr<NetDevice>`` para  um m�todo ``EnableAscii``. O ``Ptr<Node>`` � impl�cito pois o dispositivo de rede deve pertencer a exatamente um ``Node``. Por exemplo,

::

  Ptr<NetDevice> nd;
  ...
  helper.EnableAscii ("prefix", nd);

..
	The first four methods also include a default parameter called ``explicitFilename``
	that operate similar to equivalent parameters in the pcap case.

Os primeiros quatro m�todos tamb�m incluem um par�metro padr�o ``explicitFilename`` que opera similar aos par�metros no caso do pcap.

..
	In this case, no trace contexts are written to the ascii trace file since they
	would be redundant.  The system will pick the file name to be created using
	the same rules as described in the pcap section, except that the file will
	have the suffix ".tr" instead of ".pcap".

Neste caso, nenhum contexto de rastreamento � escrito para o arquivo ASCII pois seriam redundantes. O sistema pegar� o nome do arquivo para ser criado usando as mesmas regras como descritas na se��o pcap, exceto que o arquivo ter� o extens�o ".tr" ao inv�s de ".pcap".

..
	If you want to enable ascii tracing on more than one net device and have all 
	traces sent to a single file, you can do that as well by using an object to
	refer to a single file.  We have already seen this in the "cwnd" example
	above:

Para habilitar o rastreamento ASCII em mais de um dispositivo de rede e ter todos os dados de rastreamento enviados para um �nico arquivo, pode-se usar um objeto para referenciar um �nico arquivo. N�s j� verificamos isso no exemplo "cwnd":

::

  Ptr<NetDevice> nd1;
  Ptr<NetDevice> nd2;
  ...
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream 
  		("trace-file-name.tr");
  ...
  helper.EnableAscii (stream, nd1);
  helper.EnableAscii (stream, nd2);

..
	In this case, trace contexts are written to the ascii trace file since they
	are required to disambiguate traces from the two devices.  Note that since the
	user is completely specifying the file name, the string should include the ",tr"
	for consistency.

Neste caso, os contextos s�o escritos para o arquivo ASCII quando � necess�rio distinguir os dados de rastreamento de dois dispositivos. � interessante usar no nome do arquivo a extens�o ".tr" por motivos de consist�ncia.

..
	You can enable ascii tracing on a particular node/net-device pair by providing a
	``std::string`` representing an object name service string to an 
	``EnablePcap`` method.  The ``Ptr<NetDevice>`` is looked up from the name
	string.  Again, the ``<Node>`` is implicit since the named net device must 
	belong to exactly one ``Node``.  For example, 

Podemos habilitar o rastreamento ASCII em um par n�/dispositivo-rede espec�fico passando ao m�todo ``EnableAscii`` uma ``std::string`` representando um nome no servi�o de nomes de objetos. O ``Ptr<NetDevice>`` � obtido a partir do nome. Novamente, o ``<Node>`` � impl�cito pois o dispositivo de rede deve pertencer a exatamente um ``Node``. Por exemplo,

::

  Names::Add ("client" ...);
  Names::Add ("client/eth0" ...);
  Names::Add ("server" ...);
  Names::Add ("server/eth0" ...);
  ...
  helper.EnableAscii ("prefix", "client/eth0");
  helper.EnableAscii ("prefix", "server/eth0");

..
	This would result in two files named "prefix-client-eth0.tr" and 
	"prefix-server-eth0.tr" with traces for each device in the respective trace
	file.  Since all of the EnableAscii functions are overloaded to take a stream wrapper,
	you can use that form as well:

Isto resultaria em dois nomes de arquivos - "prefix-client-eth0.tr" e "prefix-server-eth0.tr" - com os rastreamentos de cada dispositivo em  seu arquivo respectivo. Como todas as fun��es do ``EnableAscii`` s�o sobrecarregadas para suportar um *stream wrapper*, podemos usar da seguinte forma tamb�m:

::

  Names::Add ("client" ...);
  Names::Add ("client/eth0" ...);
  Names::Add ("server" ...);
  Names::Add ("server/eth0" ...);
  ...
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream 
  		("trace-file-name.tr");
  ...
  helper.EnableAscii (stream, "client/eth0");
  helper.EnableAscii (stream, "server/eth0");

..
	This would result in a single trace file called "trace-file-name.tr" that 
	contains all of the trace events for both devices.  The events would be 
	disambiguated by trace context strings.

Isto resultaria em um �nico arquivo chamado "trace-file-name.tr" que cont�m todosos eventos rastreados para ambos os dispositivos. Os eventos seriam diferenciados por `strings` de contexto.

..
	You can enable ascii tracing on a collection of node/net-device pairs by 
	providing a ``NetDeviceContainer``.  For each ``NetDevice`` in the container
	the type is checked.  For each device of the proper type (the same type as is 
	managed by the device helper), tracing is enabled.    Again, the ``<Node>`` is 
	implicit since the found net device must belong to exactly one ``Node``.
	For example, 

Podemos habilitar o rastreamento ASCII em um cole��o de pares n�/dispositivo-rede fornecendo um ``NetDeviceContainer``. Para cada ``NetDevice`` no cont�iner o tipo � verificado. Para cada dispositivo de um tipo adequado (o mesmo tipo que � gerenciado por uma classe assistente de dispositivo), o rastreamento � habilitado. Novamente, o ``<Node>`` � impl�cito pois o dispositivo de rede encontrado deve pertencer a exatamente um ``Node``. 

::

  NetDeviceContainer d = ...;
  ...
  helper.EnableAscii ("prefix", d);

..
	This would result in a number of ascii trace files being created, each of which
	follows the <prefix>-<node id>-<device id>.tr convention.  Combining all of the
	traces into a single file is accomplished similarly to the examples above:

Isto resultaria em v�rios arquivos de rastreamento ASCII sendo criados, cada um seguindo a conven��o ``<prefixo>-<id do n�>-<id do dispositivo>.tr``.

Para obtermos um �nico arquivo ter�amos:

::

  NetDeviceContainer d = ...;
  ...
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream 
  		("trace-file-name.tr");
  ...
  helper.EnableAscii (stream, d);

..
	You can enable ascii tracing on a collection of node/net-device pairs by 
	providing a ``NodeContainer``.  For each ``Node`` in the ``NodeContainer``
	its attached ``NetDevices`` are iterated.  For each ``NetDevice`` attached
	to each node in the container, the type of that device is checked.  For each 
	device of the proper type (the same type as is managed by the device helper), 
	tracing is enabled.

Podemos habilitar o rastreamento ASCII em um cole��o de pares n�/dispositivo-rede fornecendo um ``NodeContainer``. Para cada ``Node`` no ``NodeContainer``, os seus ``NetDevices`` s�o percorridos. Para cada ``NetDevice`` associado a cada n� no cont�iner, o tipo do dispositivo � verificado. Para cada dispositivo do tipo adequado (o mesmo tipo que � gerenciado pelo assistente de dispositivo), o rastreamento � habilitado.

::

  NodeContainer n;
  ...
  helper.EnableAscii ("prefix", n);

..
	This would result in a number of ascii trace files being created, each of which
	follows the <prefix>-<node id>-<device id>.tr convention.  Combining all of the
	traces into a single file is accomplished similarly to the examples above:
		
Isto resultaria em v�rios arquivos ASCII sendo criados, cada um seguindo a conven��o ``<prefixo>-<id do n�>-<id do dispositivo>.tr``.

..
	You can enable pcap tracing on the basis of node ID and device ID as well as
	with explicit ``Ptr``.  Each ``Node`` in the system has an integer node ID
	and each device connected to a node has an integer device ID.
	
Podemos habilitar o rastreamento pcap na base da `ID` do n� e `ID` do dispositivo t�o bem como com um ``Ptr``. Cada ``Node`` no sistema possui um n�mero identificador inteiro associado ao n� e cada dispositivo conectado possui um n�mero identificador inteiro associado ao dispositivo.

::

  helper.EnableAscii ("prefix", 21, 1);

..
	Of course, the traces can be combined into a single file as shown above.

Os rastreamentos podem ser combinados em um �nico arquivo como mostrado acima.

..
	Finally, you can enable pcap tracing for all devices in the system, with the
	same type as that managed by the device helper.

Finalmente, podemos habilitar o rastreamento ASCII para todos os dispositivos no sistema.

::

  helper.EnableAsciiAll ("prefix");

..
	This would result in a number of ascii trace files being created, one for
	every device in the system of the type managed by the helper.  All of these
	files will follow the <prefix>-<node id>-<device id>.tr convention.  Combining
	all of the traces into a single file is accomplished similarly to the examples
	above.

Isto resultaria em v�rios arquivos ASCII sendo criados, um para cada dispositivo no sistema do tipo gerenciado pelo assistente. Todos estes arquivos seguiriam a conven��o ``<prefixo>-<id do n�>-<id do dispositivo>.tr``.

.. 
	Ascii Tracing Device Helper Filename Selection

Selecionando Nome de Arquivo para as Sa�das ASCII
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

..
	Implicit in the prefix-style method descriptions above is the construction of the
	complete filenames by the implementation method.  By convention, ascii traces
	in the |ns3| system are of the form "<prefix>-<node id>-<device id>.tr"

Impl�cito nas descri��es de m�todos anteriores � a constru��o do nome de arquivo por meio do m�todo da implementa��o. Por conven��o, rastreamento ASCII no |ns3| usa a forma "``<prefixo>-<id do n�>-<id do dispositivo>.tr``".

..
	As previously mentioned, every node in the system will have a system-assigned
	node id; and every device will have an interface index (also called a device id)
	relative to its node.  By default, then, an ascii trace file created as a result
	of enabling tracing on the first device of node 21, using the prefix "prefix",
	would be "prefix-21-1.tr".

Como mencionado, todo n� no sistema ter� um `id` de n� associado; e todo dispositivo ter� um �ndice de interface (tamb�m chamado de id do dispositivo) relativo ao seu n�. Por padr�o, ent�o, um arquivo ASCII criado como um resultado de ativar rastreamento no primeiro dispositivo do n� 21 usando o prefixo "prefix" seria "prefix-21-1.tr".

..
	You can always use the |ns3| object name service to make this more clear.
	For example, if you use the object name service to assign the name "server"
	to node 21, the resulting ascii trace file name will automatically become,
	"prefix-server-1.tr" and if you also assign the name "eth0" to the 
	device, your ascii trace file name will automatically pick this up and be called
	"prefix-server-eth0.tr".

Sempre podemos usar o servi�o de nome de objeto do |ns3| para tornar isso mais claro. Por exemplo, se usarmos o servi�o para associar o nome ``server`` ao n� 21, o arquivo ASCII resultante automaticamente ser�, ``prefix-server-1.tr`` e se tamb�m associarmos o nome ``eth0`` ao dispositivo, o nome do arquivo ASCII automaticamente ser� denominado ``prefix-server-eth0.tr``.

..
	Several of the methods have a default parameter called ``explicitFilename``.
	When set to true, this parameter disables the automatic filename completion 
	mechanism and allows you to create an explicit filename.  This option is only
	available in the methods which take a prefix and enable tracing on a single device.  
	
Diversos m�todos tem um par�metro padr�o ``explicitFilename``. Quando modificado para verdadeiro, este par�metro desabilita o mecanismo autom�tico de completar o nome do arquivo e permite criarmos um nome de arquivo abertamente. Esta op��o est� dispon�vel nos m�todos que possuam um prefixo e ativem o rastreamento em um �nico dispositivo.

.. 
	Pcap Tracing Protocol Helpers

Classes Assistentes de Protocolo para Rastreamento Pcap
++++++++++++++++++++++++++++++++++++++++++++++++++++++++

..
	The goal of these ``mixins`` is to make it easy to add a consistent pcap trace
	facility to protocols.  We want all of the various flavors of pcap tracing to 
	work the same across all protocols, so the methods of these helpers are 
	inherited by stack helpers.  Take a look at ``src/internet/helper/internet-trace-helper.h``
	if you want to follow the discussion while looking at real code.

O objetivo destes ``mixins`` � facilitar a adi��o de um mecanismo consistente para da facilidade de rastreamento para protocolos. Queremos que todos os mecanismos de rastreamento para todos os protocolos operem de mesma forma, logo os m�todos dessas classe assistentes s�o herdados por assistentes de pilha. Acesse ``src/internet/helper/internet-trace-helper.h`` para acompanhar o conte�do discutido nesta se��o.

..
	In this section we will be illustrating the methods as applied to the protocol
	``Ipv4``.  To specify traces in similar protocols, just substitute the
	appropriate type.  For example, use a ``Ptr<Ipv6>`` instead of a
	``Ptr<Ipv4>`` and call ``EnablePcapIpv6`` instead of ``EnablePcapIpv4``.

Nesta se��o ilustraremos os m�todos aplicados ao protocolo ``Ipv4``. Para especificar rastreamentos em protocolos similares, basta substituir pelo tipo apropriado. Por exemplo, use um ``Ptr<Ipv6>`` ao inv�s de um ``Ptr<Ipv4>`` e chame um ``EnablePcapIpv6`` ao inv�s de ``EnablePcapIpv4``.

..
	The class ``PcapHelperForIpv4`` provides the high level functionality for
	using pcap tracing in the ``Ipv4`` protocol.  Each protocol helper enabling these
	methods must implement a single virtual method inherited from this class.  There
	will be a separate implementation for ``Ipv6``, for example, but the only
	difference will be in the method names and signatures.  Different method names
	are required to disambiguate class ``Ipv4`` from ``Ipv6`` which are both 
	derived from class ``Object``, and methods that share the same signature.

A classe ``PcapHelperForIpv4`` prov� funcionalidade de alto n�vel para usar rastreamento no protocolo ``Ipv4``. Cada classe assistente de protocolo devem implementar um m�todo herdado desta. Haver� uma implementa��o separada para ``Ipv6``, por exemplo, mas a diferen�a ser� apenas nos nomes dos m�todos e assinaturas. Nomes de m�todos diferentes s�o necess�rio para distinguir a classe ``Ipv4`` da ``Ipv6``, pois ambas s�o derivadas da classe ``Object``, logo os m�todos compartilham a mesma assinatura.

::

  virtual void EnablePcapIpv4Internal (std::string prefix, Ptr<Ipv4> ipv4, 
  		uint32_t interface, bool explicitFilename) = 0;

..
	The signature of this method reflects the protocol and interface-centric view 
	of the situation at this level.  All of the public methods inherited from class 
	``PcapHelperForIpv4`` reduce to calling this single device-dependent
	implementation method.  For example, the lowest level pcap method,

A assinatura desse m�todo reflete a vis�o do protocolo e interface da situa��o neste n�vel. Todos os m�todos herdados da classe ``PcapHelperForIpv4`` resumem-se a chamada deste �nico m�todo dependente de dispositivo. Por exemplo, o m�todo do pcap de mais baixo n�vel, 

::

  void EnablePcapIpv4 (std::string prefix, Ptr<Ipv4> ipv4, uint32_t interface, 
  		bool explicitFilename = false);

..
	will call the device implementation of ``EnablePcapIpv4Internal`` directly.
	All other public pcap tracing methods build on this implementation to provide 
	additional user-level functionality.  What this means to the user is that all 
	protocol helpers in the system will have all of the pcap trace methods 
	available; and these methods will all work in the same way across 
	protocols if the helper implements ``EnablePcapIpv4Internal`` correctly.

chamar� a implementa��o de dispositivo de ``EnablePcapIpv4Internal`` diretamente. Todos os outros m�todos p�blicos de rastreamento pcap  s�o constru�dos a partir desta implementa��o para prover funcionalidades adicionais em n�vel do usu�rio. Para o usu�rio, isto significa que todas as classes assistentes de dispositivo no sistema ter�o todos os m�todos de rastreamento pcap dispon�veis; e estes m�todos trabalhar�o da mesma forma entre dispositivos se o dispositivo implementar corretamente ``EnablePcapIpv4Internal``.


.. 
	Pcap Tracing Protocol Helper Methods

M�todos da Classe Assistente de Protocolo para Rastreamento Pcap
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

..
	These methods are designed to be in one-to-one correspondence with the ``Node``-
	and ``NetDevice``- centric versions of the device versions.  Instead of
	``Node`` and ``NetDevice`` pair constraints, we use protocol and interface
	constraints.

Estes m�todos s�o projetados para terem correspond�ncia de um-para-um com o ``Node`` e ``NetDevice``. Ao inv�s de restri��es de pares ``Node`` e ``NetDevice``, usamos restri��es de protocolo e interface.

.. 
	Note that just like in the device version, there are six methods:

Note que como na vers�o de dispositivo, h� seis m�todos:

::

  void EnablePcapIpv4 (std::string prefix, Ptr<Ipv4> ipv4, uint32_t interface, 
  		bool explicitFilename = false);
  void EnablePcapIpv4 (std::string prefix, std::string ipv4Name, 
  		uint32_t interface, bool explicitFilename = false);
  void EnablePcapIpv4 (std::string prefix, Ipv4InterfaceContainer c);
  void EnablePcapIpv4 (std::string prefix, NodeContainer n);
  void EnablePcapIpv4 (std::string prefix, uint32_t nodeid, uint32_t interface, 
  		bool explicitFilename);
  void EnablePcapIpv4All (std::string prefix);

..
	You are encouraged to peruse the Doxygen for class ``PcapHelperForIpv4``
	to find the details of these methods; but to summarize ...

Para maiores detalhes sobre estes m�todos � interessante consultar na documenta��o da classe ``PcapHelperForIpv4``, mas para resumir ...

..
	You can enable pcap tracing on a particular protocol/interface pair by providing a
	``Ptr<Ipv4>`` and ``interface`` to an ``EnablePcap`` method.  For example, 

Podemos habilitar o rastreamento pcap em um par protocolo/interface  passando um ``Ptr<Ipv4>`` e ``interface`` para um m�todo ``EnablePcap``. Por exemplo,

::

  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  ...
  helper.EnablePcapIpv4 ("prefix", ipv4, 0);

..
	You can enable pcap tracing on a particular node/net-device pair by providing a
	``std::string`` representing an object name service string to an 
	``EnablePcap`` method.  The ``Ptr<Ipv4>`` is looked up from the name
	string.  For example, 

Podemos ativar o rastreamento pcap em um par protocolo/interface passando uma ``std::string`` que representa um nome de servi�o para um m�todo ``EnablePcapIpv4``. O ``Ptr<Ipv4>`` � buscado a partir do nome da `string`.
Por exemplo,

::

  Names::Add ("serverIPv4" ...);
  ...
  helper.EnablePcapIpv4 ("prefix", "serverIpv4", 1);

..
	You can enable pcap tracing on a collection of protocol/interface pairs by 
	providing an ``Ipv4InterfaceContainer``.  For each ``Ipv4`` / interface
	pair in the container the protocol type is checked.  For each protocol of the 
	proper type (the same type as is managed by the device helper), tracing is 
	enabled for the corresponding interface.  For example, 

Podemos ativar o rastreamento pcap em uma cole��o de pares protocolo/interface usando um ``Ipv4InterfaceContainer``. Para cada par``Ipv4``/interface no cont�iner o tipo do protocolo � verificado. Para cada protocolo do tipo adequado, o rastreamento � ativado para a interface correspondente. Por exemplo,


::

  NodeContainer nodes;
  ...
  NetDeviceContainer devices = deviceHelper.Install (nodes);
  ... 
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);
  ...
  helper.EnablePcapIpv4 ("prefix", interfaces);

..
	You can enable pcap tracing on a collection of protocol/interface pairs by 
	providing a ``NodeContainer``.  For each ``Node`` in the ``NodeContainer``
	the appropriate protocol is found.  For each protocol, its interfaces are 
	enumerated and tracing is enabled on the resulting pairs.  For example,

Podemos ativar o rastreamento em uma cole��o de pares protocolo/interface usando um ``NodeContainer``. Para cada ``Node`` no ``NodeContainer`` o protocolo apropriado � encontrado. Para cada protocolo, suas interfaces s�o enumeradas e o rastreamento � ativado nos pares resultantes. Por exemplo,

::

  NodeContainer n;
  ...
  helper.EnablePcapIpv4 ("prefix", n);

..
	You can enable pcap tracing on the basis of node ID and interface as well.  In
	this case, the node-id is translated to a ``Ptr<Node>`` and the appropriate
	protocol is looked up in the node.  The resulting protocol and interface are
	used to specify the resulting trace source.

Pode ativar o rastreamento pcap usando o n�mero identificador do n� e da interface. Neste caso, o `ID` do n� � traduzido para um ``Ptr<Node>`` e o protocolo apropriado � buscado no n�. O protocolo e interface resultante s�o usados para especificar a origem do rastreamento resultante.


::

  helper.EnablePcapIpv4 ("prefix", 21, 1);

..
	Finally, you can enable pcap tracing for all interfaces in the system, with
	associated protocol being the same type as that managed by the device helper.

Por fim, podemos ativar rastreamento pcap para todas as interfaces no sistema, desde que o protocolo seja do mesmo tipo gerenciado pela classe assistente.


::

  helper.EnablePcapIpv4All ("prefix");

.. 
	Pcap Tracing Protocol Helper Filename Selection

Sele��o de um Nome de Arquivo para o Rastreamento Pcap da Classe Assistente de Protocolo
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

..
	Implicit in all of the method descriptions above is the construction of the
	complete filenames by the implementation method.  By convention, pcap traces
	taken for devices in the |ns3| system are of the form 
	"<prefix>-<node id>-<device id>.pcap".  In the case of protocol traces,
	there is a one-to-one correspondence between protocols and ``Nodes``.
	This is because protocol ``Objects`` are aggregated to ``Node Objects``.
	Since there is no global protocol id in the system, we use the corresponding
	node id in file naming.  Therefore there is a possibility for file name 
	collisions in automatically chosen trace file names.  For this reason, the
	file name convention is changed for protocol traces.

Impl�cito nas descri��es de m�todos anterior � a constru��o do nome de arquivo por meio do m�todo da implementa��o. Por conven��o, rastreamento pcap no |ns3| usa a forma ``<prefixo>-<id do n�>-<id do dispositivo>.pcap``. No caso de rastreamento de protocolos, h� uma correspond�ncia de um-para-um entre protocolos e ``Nodes``. Isto porque ``Objects`` de protocolo s�o agregados a `Node Objects``. Como n�o h� um `id` global de protocolo no sistema, usamos o `ID` do n� na nomea��o do arquivo. Consequentemente h� possibilidade de colis�o de nomes quando usamos o sistema autom�tico de nomes. Por esta raz�o, a conven��o de nome de arquivo � modificada para rastreamentos de protocolos.

..
	As previously mentioned, every node in the system will have a system-assigned
	node id.  Since there is a one-to-one correspondence between protocol instances
	and node instances we use the node id.  Each interface has an interface id 
	relative to its protocol.  We use the convention 
	"<prefix>-n<node id>-i<interface id>.pcap" for trace file naming in protocol
	helpers.

Como mencionado, todo n� no sistema ter� um `id` de n� associado. Como h� uma correspond�ncia de um-para-um entre inst�ncias de protocolo e inst�ncias de n�, usamos o `id` de n�. Cada interface tem um `id` de interface relativo ao seu protocolo. Usamos a conven��o "<prefixo>-n<id do n�>-i<id da interface>.pcap" para especificar o nome do arquivo de rastreamento para as classes assistentes de protocolo.

..
	Therefore, by default, a pcap trace file created as a result of enabling tracing
	on interface 1 of the Ipv4 protocol of node 21 using the prefix "prefix"
	would be "prefix-n21-i1.pcap".

Consequentemente, por padr�o, uma arquivo pcap criado como um resultado da ativa��o de rastreamento na interface 1 do protocolo ipv4 do n� 21 usando o prefixo ``prefix`` seria ``prefix-n21-i1.pcap``.

..
	You can always use the |ns3| object name service to make this more clear.
	For example, if you use the object name service to assign the name "serverIpv4"
	to the Ptr<Ipv4> on node 21, the resulting pcap trace file name will 
	automatically become, "prefix-nserverIpv4-i1.pcap".

Sempre podemos usar o servi�o de nomes de objetos do |ns3| para tornar isso mais claro. Por exemplo, se usamos o servi�o de nomes  para associar o nome "serverIpv4" ao Ptr<Ipv4> no n� 21, o nome de arquivo resultante seria ``prefix-nserverIpv4-i1.pcap``.

..
	Several of the methods have a default parameter called ``explicitFilename``.
	When set to true, this parameter disables the automatic filename completion 
	mechanism and allows you to create an explicit filename.  This option is only
	available in the methods which take a prefix and enable tracing on a single device.  

Diversos m�todos tem um par�metro padr�o ``explicitFilename``. Quando modificado para verdadeiro, este par�metro desabilita o mecanismo autom�tico de completar o nome do arquivo e permite criarmos um nome de arquivo abertamente. Esta op��o est� dispon�vel nos m�todos que  ativam o rastreamento pcap em um �nico dispositivo.

.. 
	Ascii Tracing Protocol Helpers

Classes Assistentes de Protocolo para Rastreamento ASCII
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++

..
	The behavior of the ascii trace helpers is substantially similar to the pcap
	case.  Take a look at ``src/internet/helper/internet-trace-helper.h`` if you want to 
	follow the discussion while looking at real code.

O comportamento dos assistentes de rastreamento ASCII � similar ao do pcap. Acesse o arquivo ``src/internet/helper/internet-trace-helper.h`` para compreender melhor o funcionamento dessa classe assistente.

..
	In this section we will be illustrating the methods as applied to the protocol
	``Ipv4``.  To specify traces in similar protocols, just substitute the
	appropriate type.  For example, use a ``Ptr<Ipv6>`` instead of a
	``Ptr<Ipv4>`` and call ``EnableAsciiIpv6`` instead of ``EnableAsciiIpv4``.

Nesta se��o apresentamos os m�todos aplicados ao protocolo ``Ipv4``. Para protocolos similares apenas substitua para o tipo apropriado. Por exemplo, use um ``Ptr<Ipv6>`` ao inv�s de um  ``Ptr<Ipv4>`` e chame ``EnableAsciiIpv6`` ao inv�s de ``EnableAsciiIpv4``.

..
	The class ``AsciiTraceHelperForIpv4`` adds the high level functionality
	for using ascii tracing to a protocol helper.  Each protocol that enables these
	methods must implement a single virtual method inherited from this class.

A classe ``AsciiTraceHelperForIpv4`` adiciona funcionalidade de alto n�vel para usar rastreamento ASCII para um assistente de protocolo. Todo protocolo que usa estes m�todos deve implementar um m�todo herdado desta classe. 

::

  virtual void EnableAsciiIpv4Internal (Ptr<OutputStreamWrapper> stream, 
                                        std::string prefix, 
                                        Ptr<Ipv4> ipv4, 
                                        uint32_t interface,
                                        bool explicitFilename) = 0;

..
	The signature of this method reflects the protocol- and interface-centric view 
	of the situation at this level; and also the fact that the helper may be writing
	to a shared output stream.  All of the public methods inherited from class 
	``PcapAndAsciiTraceHelperForIpv4`` reduce to calling this single device-
	dependent implementation method.  For example, the lowest level ascii trace
	methods,

A assinatura deste m�todo reflete a vis�o central do protocolo e interface da situa��o neste n�vel; e tamb�m o fato que o assistente pode ser escrito para um fluxo de sa�da compartilhado. Todos os m�todos p�blicos herdados desta classe ``PcapAndAsciiTraceHelperForIpv4`` resumem-se a chamada deste �nico m�todo dependente de implementa��o. Por exemplo, os m�todos de rastreamento ASCII de mais baixo n�vel,

::

  void EnableAsciiIpv4 (std::string prefix, Ptr<Ipv4> ipv4, uint32_t interface, 
  		bool explicitFilename = false);
  void EnableAsciiIpv4 (Ptr<OutputStreamWrapper> stream, Ptr<Ipv4> ipv4, 
  		uint32_t interface);

..
	will call the device implementation of ``EnableAsciiIpv4Internal`` directly,
	providing either the prefix or the stream.  All other public ascii tracing 
	methods will build on these low-level functions to provide additional user-level
	functionality.  What this means to the user is that all device helpers in the 
	system will have all of the ascii trace methods available; and these methods
	will all work in the same way across protocols if the protocols implement 
	``EnablAsciiIpv4Internal`` correctly.

chamar�o uma implementa��o de ``EnableAsciiIpv4Internal`` diretamente, passando um prefixo ou fluxo v�lido. Todos os outros m�todos p�blicos ser�o constru�dos a partir destas fun��es de baixo n�vel para fornecer funcionalidades adicionais em n�vel de usu�rio. Para o usu�rio, isso significa que todos os assistentes de protocolos no sistema ter�o todos os m�todos de rastreamento ASCII dispon�veis e estes m�todos trabalhar�o do mesmo modo em todos os protocolos se estes implementarem ``EnableAsciiIpv4Internal``.


.. 
	Ascii Tracing Protocol Helper Methods

M�todos da Classe Assistente de Protocolo para Rastreamento ASCII
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

  void EnableAsciiIpv4 (std::string prefix, Ptr<Ipv4> ipv4, uint32_t interface, 
                        bool explicitFilename = false);
  void EnableAsciiIpv4 (Ptr<OutputStreamWrapper> stream, Ptr<Ipv4> ipv4,
                        uint32_t interface);

  void EnableAsciiIpv4 (std::string prefix, std::string ipv4Name, uint32_t interface,
                        bool explicitFilename = false);
  void EnableAsciiIpv4 (Ptr<OutputStreamWrapper> stream, std::string ipv4Name,
                        uint32_t interface);

  void EnableAsciiIpv4 (std::string prefix, Ipv4InterfaceContainer c);
  void EnableAsciiIpv4 (Ptr<OutputStreamWrapper> stream, Ipv4InterfaceContainer c);

  void EnableAsciiIpv4 (std::string prefix, NodeContainer n);
  void EnableAsciiIpv4 (Ptr<OutputStreamWrapper> stream, NodeContainer n);

  void EnableAsciiIpv4All (std::string prefix);
  void EnableAsciiIpv4All (Ptr<OutputStreamWrapper> stream);

  void EnableAsciiIpv4 (std::string prefix, uint32_t nodeid, uint32_t deviceid,
                        bool explicitFilename);
  void EnableAsciiIpv4 (Ptr<OutputStreamWrapper> stream, uint32_t nodeid, 
                        uint32_t interface);

..
	You are encouraged to peruse the Doxygen for class ``PcapAndAsciiHelperForIpv4``
	to find the details of these methods; but to summarize ...

Para maiores detalhes sobre os m�todos consulte na documenta��o a classe ``PcapAndAsciiHelperForIpv4``; mas para resumir ...

..
	There are twice as many methods available for ascii tracing as there were for
	pcap tracing.  This is because, in addition to the pcap-style model where traces
	from each unique protocol/interface pair are written to a unique file, we 
	support a model in which trace information for many protocol/interface pairs is 
	written to a common file.  This means that the <prefix>-n<node id>-<interface>
	file name generation mechanism is replaced by a mechanism to refer to a common 
	file; and the number of API methods is doubled to allow all combinations.

H� duas vezes mais m�todos dispon�veis para rastreamento ASCII que para rastreamento pcap. Isto ocorre pois para o modelo pcap os rastreamentos de cada par protocolo/interface s�o escritos para um �nico arquivo, enquanto que no ASCII todo as as informa��es s�o escritas para um arquivo comum. Isto significa que o mecanismo de gera��o de nomes de arquivos "<prefixo>-n<id do n�>-i<interface>" � substitu�do por um mecanismo para referenciar um arquivo comum; e o n�mero de m�todos da API � duplicado para permitir todas as combina��es.

..
	Just as in pcap tracing, you can enable ascii tracing on a particular 
	protocol/interface pair by providing a ``Ptr<Ipv4>`` and an ``interface``
	to an ``EnableAscii`` method.
	For example, 

Assim, como no rastreamento pcap, podemos ativar o rastreamento ASCII em um par protocolo/interface passando um ``Ptr<Ipv4>`` e uma ``interface`` para  um m�todo ``EnableAsciiIpv4``. Por exemplo,


::

  Ptr<Ipv4> ipv4;
  ...
  helper.EnableAsciiIpv4 ("prefix", ipv4, 1);

..
	In this case, no trace contexts are written to the ascii trace file since they
	would be redundant.  The system will pick the file name to be created using
	the same rules as described in the pcap section, except that the file will
	have the suffix ".tr" instead of ".pcap".

Neste caso, nenhum contexto de rastreamento � escrito para o arquivo ASCII pois seriam redundantes. O sistema pegar� o nome do arquivo para ser criado usando as mesmas regras como descritas na se��o pcap, exceto que o arquivo ter� o extens�o ``.tr`` ao inv�s de ``.pcap``.

..
	If you want to enable ascii tracing on more than one interface and have all 
	traces sent to a single file, you can do that as well by using an object to
	refer to a single file.  We have already something similar to this in the
	"cwnd" example above:

Para habilitar o rastreamento ASCII em mais de uma interface e ter todos os dados de rastreamento enviados para um �nico arquivo, pode-se usar um objeto para referenciar um �nico arquivo. N�s j� verificamos isso no exemplo "cwnd":

::

  Ptr<Ipv4> protocol1 = node1->GetObject<Ipv4> ();
  Ptr<Ipv4> protocol2 = node2->GetObject<Ipv4> ();
  ...
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream 
  		("trace-file-name.tr");
  ...
  helper.EnableAsciiIpv4 (stream, protocol1, 1);
  helper.EnableAsciiIpv4 (stream, protocol2, 1);

..
	In this case, trace contexts are written to the ascii trace file since they
	are required to disambiguate traces from the two interfaces.  Note that since 
	the user is completely specifying the file name, the string should include the
	",tr" for consistency.

Neste caso, os contextos s�o escritos para o arquivo ASCII quando � necess�rio distinguir os dados de rastreamento de duas interfaces. � interessante usar no nome do arquivo a extens�o ``.tr`` por motivos de consist�ncia.

..
	You can enable ascii tracing on a particular protocol by providing a 
	``std::string`` representing an object name service string to an 
	``EnablePcap`` method.  The ``Ptr<Ipv4>`` is looked up from the name
	string.  The ``<Node>`` in the resulting filenames is implicit since there
	is a one-to-one correspondence between protocol instances and nodes,
	For example, 

Pode habilitar o rastreamento ASCII em protocolo espec�fico passando ao m�todo ``EnableAsciiIpv4`` uma ``std::string`` representando um nome no servi�o de nomes de objetos. O ``Ptr<Ipv4>`` � obtido a partir do nome. O ``<Node>`` � impl�cito, pois h� uma correspond�ncia de um-para-um entre instancias de protocolos e n�s. Por exemplo,

::

  Names::Add ("node1Ipv4" ...);
  Names::Add ("node2Ipv4" ...);
  ...
  helper.EnableAsciiIpv4 ("prefix", "node1Ipv4", 1);
  helper.EnableAsciiIpv4 ("prefix", "node2Ipv4", 1);

..
	This would result in two files named "prefix-nnode1Ipv4-i1.tr" and 
	"prefix-nnode2Ipv4-i1.tr" with traces for each interface in the respective 
	trace file.  Since all of the EnableAscii functions are overloaded to take a 
	stream wrapper, you can use that form as well:

Isto resultaria em dois nomes de arquivos ``prefix-nnode1Ipv4-i1.tr`` e ``prefix-nnode2Ipv4-i1.tr``, com os rastreamentos de cada interface em  seu arquivo respectivo. Como todas as fun��es do ``EnableAsciiIpv4`` s�o sobrecarregadas para suportar um *stream wrapper*, podemos usar da seguinte forma tamb�m:


::

  Names::Add ("node1Ipv4" ...);
  Names::Add ("node2Ipv4" ...);
  ...
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream 
  		("trace-file-name.tr");
  ...
  helper.EnableAsciiIpv4 (stream, "node1Ipv4", 1);
  helper.EnableAsciiIpv4 (stream, "node2Ipv4", 1);

..
	This would result in a single trace file called "trace-file-name.tr" that 
	contains all of the trace events for both interfaces.  The events would be 
	disambiguated by trace context strings.

Isto resultaria em um �nico arquivo chamado ``trace-file-name.tr`` que cont�m todos os eventos rastreados para ambas as interfaces. Os eventos seriam diferenciados por `strings` de contexto.

.. 
	You can enable ascii tracing on a collection of protocol/interface pairs by 
	providing an ``Ipv4InterfaceContainer``.  For each protocol of the proper 
	type (the same type as is managed by the device helper), tracing is enabled
	for the corresponding interface.  Again, the ``<Node>`` is implicit since 
	there is a one-to-one correspondence between each protocol and its node.
	For example, 

Podemos habilitar o rastreamento ASCII em um cole��o de pares protocolo/interface provendo um ``Ipv4InterfaceContainer``. Para cada protocolo no cont�iner o tipo � verificado. Para cada protocolo do tipo adequado (o mesmo tipo que � gerenciado por uma classe assistente de protocolo), o rastreamento � habilitado para a interface correspondente. Novamente, o ``<Node>`` � impl�cito, pois h� uma correspond�ncia de um-para-um entre protocolo e seu n�. Por exemplo,


::

  NodeContainer nodes;
  ...
  NetDeviceContainer devices = deviceHelper.Install (nodes);
  ... 
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);
  ...
  ...
  helper.EnableAsciiIpv4 ("prefix", interfaces);

..
	This would result in a number of ascii trace files being created, each of which
	follows the <prefix>-n<node id>-i<interface>.tr convention.  Combining all of the
	traces into a single file is accomplished similarly to the examples above:

Isto resultaria em v�rios arquivos de rastreamento ASCII sendo criados, cada um seguindo a conven��o ``<prefixo>-n<id do n�>-i<interface>.tr``. 

Para obtermos um �nico arquivo ter�amos:

::

  NodeContainer nodes;
  ...
  NetDeviceContainer devices = deviceHelper.Install (nodes);
  ... 
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);
  ...
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream 
  		("trace-file-name.tr");
  ...
  helper.EnableAsciiIpv4 (stream, interfaces);

..
	You can enable ascii tracing on a collection of protocol/interface pairs by 
	providing a ``NodeContainer``.  For each ``Node`` in the ``NodeContainer``
	the appropriate protocol is found.  For each protocol, its interfaces are 
	enumerated and tracing is enabled on the resulting pairs.  For example,

Podemos habilitar o rastreamento ASCII em uma cole��o de pares protocolo/interface provendo um `NodeContainer``. Para cada ``Node`` no ``NodeContainer`` os protocolos apropriados s�o encontrados. Para cada protocolo, sua interface � enumerada e o rastreamento � habilitado nos pares. Por exemplo,

::

  NodeContainer n;
  ...
  helper.EnableAsciiIpv4 ("prefix", n);

..
	You can enable pcap tracing on the basis of node ID and device ID as well.  In
	this case, the node-id is translated to a ``Ptr<Node>`` and the appropriate
	protocol is looked up in the node.  The resulting protocol and interface are
	used to specify the resulting trace source.

Podemos habilitar o rastreamento pcap usando o n�mero identificador do n� e n�mero identificador do dispositivo. Neste caso, o `ID` do n� � traduzido para um ``Ptr<Node>`` e o protocolo apropriado � procurado no n� de rede. O protocolo e interface resultantes s�o usados para especificar a origem do rastreamento.

::

  helper.EnableAsciiIpv4 ("prefix", 21, 1);

.. 
	Of course, the traces can be combined into a single file as shown above.

Os rastreamentos podem ser combinados em um �nico arquivo como mostrado anteriormente.

.. 
	Finally, you can enable ascii tracing for all interfaces in the system, with
	associated protocol being the same type as that managed by the device helper.

Finalmente, podemos habilitar o rastreamento ASCII para todas as interfaces no sistema.

::

  helper.EnableAsciiIpv4All ("prefix");

..
	This would result in a number of ascii trace files being created, one for
	every interface in the system related to a protocol of the type managed by the
	helper.  All of these files will follow the <prefix>-n<node id>-i<interface.tr
	convention.  Combining all of the traces into a single file is accomplished 
	similarly to the examples above.

Isto resultaria em v�rios arquivos ASCII sendo criados, um para cada interface no sistema relacionada ao protocolo do tipo gerenciado pela classe assistente.Todos estes arquivos seguiriam a conven��o
``<prefix>-n<id do node>-i<interface>.tr``.


.. 
	Ascii Tracing Protocol Helper Filename Selection

Sele��o de Nome de Arquivo para Rastreamento ASCII da Classe Assistente de Protocolo 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

..
	Implicit in the prefix-style method descriptions above is the construction of the
	complete filenames by the implementation method.  By convention, ascii traces
	in the |ns3| system are of the form "<prefix>-<node id>-<device id>.tr"

Impl�cito nas descri��es de m�todos anteriores � a constru��o do nome do arquivo por meio do m�todo da implementa��o. Por conven��o, rastreamento ASCII no sistema |ns3| s�o da forma ``<prefix>-<id node>-<id do dispositivo>.tr``.

..
	As previously mentioned, every node in the system will have a system-assigned
	node id.  Since there is a one-to-one correspondence between protocols and nodes
	we use to node-id to identify the protocol identity.  Every interface on a 
	given protocol will have an interface index (also called simply an interface) 
	relative to its protocol.  By default, then, an ascii trace file created as a result
	of enabling tracing on the first device of node 21, using the prefix "prefix",
	would be "prefix-n21-i1.tr".  Use the prefix to disambiguate multiple protocols
	per node.

Como mencionado, todo n� no sistema ter� um n�mero identificador de n� associado. Como h� uma correspond�ncia de um-para-um entre inst�ncias de protocolo e inst�ncias de n�, usamos o `ID` de n�. Cada interface em um protocolo ter� um �ndice de interface (tamb�m chamando apenas de interface) relativo ao seu protocolo. Por padr�o, ent�o, um arquivo de rastreamento ASCII criado a partir do rastreamento no primeiro dispositivo do n� 21, usando o prefixo "prefix", seria ``prefix-n21-i1.tr``. O uso de prefixo distingue m�ltiplos protocolos por n�.

..
	You can always use the |ns3| object name service to make this more clear.
	For example, if you use the object name service to assign the name "serverIpv4"
	to the protocol on node 21, and also specify interface one, the resulting ascii 
	trace file name will automatically become, "prefix-nserverIpv4-1.tr".

Sempre podemos usar o servi�o de nomes de objetos do |ns3| para tornar isso mais claro. Por exemplo, se usarmos o servi�o de nomes para associar o nome "serverIpv4" ao Ptr<Ipv4> no n� 21, o nome de arquivo resultante seria ``prefix-nserverIpv4-i1.tr``.

..
	Several of the methods have a default parameter called ``explicitFilename``.
	When set to true, this parameter disables the automatic filename completion 
	mechanism and allows you to create an explicit filename.  This option is only
	available in the methods which take a prefix and enable tracing on a single device.  

Diversos m�todos tem um par�metro padr�o ``explicitFilename``. Quando modificado para verdadeiro, este par�metro desabilita o mecanismo autom�tico de completar o nome do arquivo e permite criarmos um nome de arquivo abertamente. Esta op��o est� dispon�vel nos m�todos que  ativam o rastreamento em um �nico dispositivo.


.. 
	Summary

Considera��es Finais
********************

..
	|ns3| includes an extremely rich environment allowing users at several 
	levels to customize the kinds of information that can be extracted from 
	simulations.  

O |ns3| inclui um ambiente completo para permitir usu�rios de diversos n�veis  personalizar os tipos de informa��o para serem extra�das de suas simula��es.

..
	There are high-level helper functions that allow users to simply control the 
	collection of pre-defined outputs to a fine granularity.  There are mid-level
	helper functions to allow more sophisticated users to customize how information
	is extracted and saved; and there are low-level core functions to allow expert
	users to alter the system to present new and previously unexported information
	in a way that will be immediately accessible to users at higher levels.

Existem fun��es assistentes de alto n�vel que permitem ao usu�rio o controle de um cole��o de sa�das predefinidas para uma granularidade mais fina. Existem fun��es assistentes de n�vel intermedi�rio que permitem usu�rios mais sofisticados personalizar como as informa��es s�o extra�das e armazenadas; e existem fun��es de baixo n�vel que permitem usu�rios avan�ados alterarem o sistema para apresentar novas ou informa��es que n�o eram exportadas.

..
	This is a very comprehensive system, and we realize that it is a lot to 
	digest, especially for new users or those not intimately familiar with C++
	and its idioms.  We do consider the tracing system a very important part of
	|ns3| and so recommend becoming as familiar as possible with it.  It is
	probably the case that understanding the rest of the |ns3| system will
	be quite simple once you have mastered the tracing system

Este � um sistema muito abrangente e percebemos que � muita informa��o para digerir, especialmente para novos usu�rios ou aqueles que n�o est�o intimamente familiarizados com C++ e suas express�es idiom�ticas. Consideramos o sistema de rastreamento uma parte muito importante do |ns3|, assim recomendamos que familiarizem-se o m�ximo poss�vel com ele. Compreender o restante do sistema |ns3| � bem simples, uma vez que dominamos o sistema de rastreamento.

