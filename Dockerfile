FROM archlinux:latest AS build

RUN pacman --noconfirm -Syu; \
    pacman --noconfirm -Sy --needed git base-devel sudo; \
    useradd -m -r -s /bin/bash aur; \
    passwd -d aur; \
    echo "aur ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers; \
    sudo -u aur git clone https://aur.archlinux.org/yay.git /home/aur/yay; \
    cd /home/aur/yay && yes | sudo -u aur makepkg -si; \
    rm -rf /home/aur/yay

RUN sudo -u aur yay --noconfirm -S mingw-w64-cmake

RUN pacman -Qtdq | xargs -r pacman --noconfirm -Rcns; \
    rm -rf /home/aur/.cache

CMD ["/bin/bash"]

FROM build AS development
